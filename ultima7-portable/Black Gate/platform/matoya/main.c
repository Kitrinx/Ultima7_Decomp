/* Process entry, window and threads.
 *
 * The process main thread runs the window (libmatoya wants it there). The game runs
 * GameMain on its own thread, and sound renders on a third.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

#define WINDOW_TITLE "Ultima VII: The Black Gate"

static MTY_App *app;
static MTY_Window window = -1;
static MTY_Time start_time;
static _Thread_local bool on_app_thread;

static MTY_Atomic32 exit_claimed;
static MTY_Atomic32 exit_requested;
static MTY_Atomic32 exit_code;
static MTY_Atomic32 fatal_claimed;
static MTY_Atomic32 fatal_pending;
static char fatal_text[1024];

static int game_argc;
static char **game_argv;

/* The window's client size, for mapping scripted mouse positions on the game thread. */
static MTY_Mutex *view_lock;
static MTY_Size view_size = {3 * 320, 3 * 240};
static bool backgrounded;

MTY_Size backend_view_size(void)
{
	MTY_Size size;

	MTY_MutexLock(view_lock);
	size = view_size;
	MTY_MutexUnlock(view_lock);
	return size;
}

#ifdef __APPLE__
#include <objc/message.h>
#include <objc/runtime.h>

/* Scripted test runs stay out of the user's way: no Dock icon, no focus, window hidden. */
static void run_in_background(void)
{
	id nsapp = ((id (*)(id, SEL)) objc_msgSend)((id) objc_getClass("NSApplication"),
		sel_registerName("sharedApplication"));

	((void (*)(id, SEL, long)) objc_msgSend)(nsapp, sel_registerName("setActivationPolicy:"), 2L);
	((void (*)(id, SEL, id)) objc_msgSend)(nsapp, sel_registerName("hide:"), (id) 0);
}
#else
static void run_in_background(void)
{
}
#endif

int64_t backend_elapsed_us(void)
{
	return (int64_t) (MTY_TimeDiff(start_time, MTY_GetTime()) * 1000.0);
}

void backend_request_exit(int code)
{
	if (MTY_Atomic32CAS(&exit_claimed, 0, 1)) {
		MTY_Atomic32Set(&exit_code, code);
		MTY_Atomic32Set(&exit_requested, 1);
	}
}

/* A thread that asked the app thread to end the process waits here for it. */
static void wait_for_exit(void)
{
	for (;;)
		MTY_Sleep(1000);
}

static void end_process(int code)
{
	audio_stop();
	fflush(NULL);
	/* Skips static destructors, which could run under a still-live game thread. */
	_Exit(code);
}

void plat_exit(int16_t code)
{
	if (on_app_thread)
		end_process(code);
	backend_request_exit(code);
	wait_for_exit();
}

void plat_fatal(const char *message)
{
	fprintf(stderr, "u7: %s\n", message);
	if (on_app_thread) {
		/* A scripted run has no one to close the box. */
		if (MTY_HasDialogs() && !test_input_active())
			MTY_ShowMessageBox(WINDOW_TITLE, "%s", message);
		end_process(1);
	}
	/* Message boxes must be shown from the app thread. */
	if (MTY_Atomic32CAS(&fatal_claimed, 0, 1)) {
		snprintf(fatal_text, sizeof fatal_text, "%s", message);
		MTY_Atomic32Set(&fatal_pending, 1);
	}
	wait_for_exit();
}

void plat_log(const char *text)
{
	fputs(text, stderr);
	fflush(stderr);
}

static bool point_over_image(int32_t x, int32_t y)
{
	int32_t vx, vy, vw, vh;

	video_view_rect(MTY_WindowGetSize(app, window), &vx, &vy, &vw, &vh);
	return x >= vx && y >= vy && x < vx + vw && y < vy + vh;
}

/* A fully transparent 1x1 PNG. Over the game image the OS pointer is this, since the game
 * draws its own; outside the window macOS shows its normal arrow by itself. */
static const uint8_t blank_cursor[] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1f, 0x15, 0xc4, 0x89, 0x00, 0x00, 0x00,
	0x0b, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0x63, 0x60, 0x00, 0x02, 0x00,
	0x00, 0x05, 0x00, 0x01, 0x7a, 0x5e, 0xab, 0x3f, 0x00, 0x00, 0x00, 0x00,
	0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82,
};
static bool pointer_over_image;

static void on_event(const MTY_Event *event, void *opaque)
{
	(void) opaque;

	switch (event->type) {
	case MTY_EVENT_CLOSE:
	case MTY_EVENT_QUIT:
	case MTY_EVENT_SHUTDOWN:
		backend_request_exit(0);
		return;
	case MTY_EVENT_MOTION:
		/* The game draws its own pointer over the image. */
		if (!event->motion.relative) {
			bool over = point_over_image(event->motion.x, event->motion.y);

			if (over != pointer_over_image) {
				pointer_over_image = over;
				MTY_AppSetCursor(app, over ? MTY_CURSOR_NONE : MTY_CURSOR_ARROW);
			}
		}
		break;
	default:
		break;
	}
	events_push(event, MTY_WindowGetSize(app, window));
}

static bool on_frame(void *opaque)
{
	uint32_t x, y;

	(void) opaque;
	if (MTY_Atomic32Get(&fatal_pending)) {
		if (MTY_HasDialogs() && !test_input_active())
			MTY_ShowMessageBox(WINDOW_TITLE, "%s", fatal_text);
		MTY_Atomic32Set(&exit_code, 1);
		return false;
	}
	if (MTY_Atomic32Get(&exit_requested))
		return false;
	MTY_MutexLock(view_lock);
	view_size = MTY_WindowGetSize(app, window);
	MTY_MutexUnlock(view_lock);
	if (test_input_active() && !backgrounded) {
		backgrounded = true;
		run_in_background();
	}
	if (events_take_warp(MTY_WindowGetSize(app, window), &x, &y))
		MTY_WindowWarpCursor(app, window, x, y);
	video_draw(app, window);
	/* Presenting does not wait for a display refresh while the window is hidden. */
	if (!MTY_WindowIsVisible(app, window))
		MTY_Sleep(16);
	return true;
}

static void *game_thread(void *opaque)
{
	(void) opaque;
	plat_exit(GameMain((int16_t) game_argc, game_argv));
	return NULL;
}

/* Takes "--data <dir>" out of the arguments the game sees. */
static const char *take_data_dir(int argc, char **argv)
{
	const char *dir = getenv("U7_DATA");

	game_argv = calloc((size_t) argc + 1, sizeof *game_argv);
	for (int i = 0; i < argc; i++) {
		if (i > 0 && strcmp(argv[i], "--data") == 0 && i + 1 < argc)
			dir = argv[++i];
		else
			game_argv[game_argc++] = argv[i];
	}
	return dir != NULL && dir[0] != '\0' ? dir : ".";
}

int main(int argc, char **argv)
{
	MTY_Frame frame;
	char problem[1024];

	on_app_thread = true;
	nullpage_install();
	start_time = MTY_GetTime();
	files_set_root(take_data_dir(argc, argv));
	if (!files_check_data(problem, sizeof problem))
		plat_fatal(problem);
	events_init();
	video_init();
	view_lock = MTY_MutexCreate();

	app = MTY_AppCreate(0, on_frame, on_event, NULL);
	if (app == NULL)
		plat_fatal("Could not start the window system.");
	frame = MTY_MakeDefaultFrame(0, 0, 3 * 320, 3 * 240, 0.9f);
	window = MTY_WindowCreate(app, WINDOW_TITLE, &frame, 0);
	MTY_AppSetPNGCursor(app, blank_cursor, sizeof blank_cursor, 0, 0);
	MTY_AppSetCursor(app, MTY_CURSOR_ARROW);
	if (window < 0 || !MTY_WindowSetGFX(app, window, MTY_GetDefaultGFX(), true))
		plat_fatal("Could not open the game window.");
	MTY_WindowSetMinSize(app, window, 320, 240);

	audio_start();
	MTY_ThreadDetach(game_thread, NULL);
	MTY_AppRun(app);

	MTY_AppDestroy(&app);
	end_process(MTY_Atomic32Get(&exit_code));
	return 0;
}
