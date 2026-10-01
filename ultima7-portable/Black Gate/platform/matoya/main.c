/* Process entry, window and threads.
 *
 * The process main thread runs the window (libmatoya wants it there). A launcher thread runs
 * Ultima7Main, or the one program named, and each program runs on a game thread of its own.
 * Sound renders on another.
 *
 *   launcher: plat_run_program --reset--> game thread: ProgramMain ... plat_exit(code)
 *                  ^                                                       |
 *                  +------------------- code (thread parked) --------------+
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
static _Thread_local bool on_game_thread;

static MTY_Atomic32 exit_claimed;
static MTY_Atomic32 exit_requested;
static MTY_Atomic32 exit_code;
static MTY_Atomic32 fatal_claimed;
static MTY_Atomic32 fatal_pending;
static char fatal_text[1024];

static int game_argc;
static char **game_argv;
static const char *program_name;

/* A program's exit code, handed to the launcher waiting in plat_run_program. */
static MTY_Waitable *program_done;
static int16_t program_code;

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
	if (on_game_thread) {
		/* As a DOS exit: the program's stack is left as it is, never unwound. */
		audio_drop_lock();
		program_code = (int16_t) (code & 0xff);
		MTY_WaitableSignal(program_done);
		wait_for_exit();
	}
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
	case MTY_EVENT_KEY:
		/* Alt-Enter toggles fullscreen and never reaches the game. */
		if ((event->key.key == MTY_KEY_ENTER || event->key.key == MTY_KEY_NP_ENTER)
			&& (event->key.mod & MTY_MOD_ALT)) {
			if (event->key.pressed)
				MTY_WindowSetFullscreen(app, window, !MTY_WindowIsFullscreen(app, window));
			return;
		}
		break;
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

typedef struct {
	const char *name;
	int16_t argc;
	char **argv;
} program_start;

static void *game_thread(void *opaque)
{
	program_start *start = opaque;

	on_game_thread = true;
	plat_exit(ProgramMain(start->name, start->argc, start->argv));
	return NULL;
}

int16_t plat_run_program(const char *name, int16_t argc, char **argv)
{
	program_start start = {name, (int16_t) (argc + 1), calloc((size_t) argc + 2, sizeof (char *))};

	/* The program sees this executable as its argv[0], as a DOS program saw its EXE. */
	start.argv[0] = game_argv[0];
	memcpy(start.argv + 1, argv, (size_t) argc * sizeof *argv);

	events_reset();
	video_reset();
	audio_reset();
	files_reset();
	console_reset();
	nullpage_reset();
	ResetEnvironment();
#ifdef U7_RESET_CHECK
	reset_check();
#endif
	MTY_ThreadDetach(game_thread, &start);
	MTY_WaitableWait(program_done, -1);
	free(start.argv);
	return program_code;
}

static void *launcher_thread(void *opaque)
{
	int16_t code;

	(void) opaque;
	if (program_name == NULL)
		code = Ultima7Main((int16_t) game_argc, game_argv);
	else
		code = plat_run_program(program_name, (int16_t) (game_argc - 1), game_argv + 1);
	backend_request_exit(code);
	return NULL;
}

/* Takes "--data <dir>" and "--program <name>" out of the arguments the game sees. */
static void show_help(void)
{
	printf("Usage: Ultima7 [--data <dir>] [switches]\n"
		"\n"
		"  --data <dir>        the game folder (default: U7_DATA, else the current folder if it\n"
		"                      has STATIC, else the folder Ultima7 is in)\n"
		"  --program <name>    run one program alone: u7 (add -p), mainmenu, intro, endgame\n"
		"  --help              this list\n"
		"\n"
		"The game's own options:\n"
		"  --cheat             enable the cheat keys\n"
		"  --cheat-start       with --cheat: move at once, move anything, an Avatar that can't\n"
		"                      die, debug output\n"
		"  --speech            speech on\n"
		"  --adlib[=port]      AdLib music (this port plays every score on the MT-32)\n"
		"  --roland[=n]        Roland MT-32 music\n"
		"  --shape-pool=<KB>   size of the shape cache\n"
		"  --overlay-size      report the DOS overlay buffer size, then stop\n"
		"  --version           show the version, then stop\n");
	exit(0);
}

static const char *take_data_dir(int argc, char **argv)
{
	const char *dir = getenv("U7_DATA");

	game_argv = calloc((size_t) argc + 1, sizeof *game_argv);
	for (int i = 0; i < argc; i++) {
		if (i > 0 && strcmp(argv[i], "--data") == 0 && i + 1 < argc)
			dir = argv[++i];
		else if (i > 0 && strcmp(argv[i], "--program") == 0 && i + 1 < argc)
			program_name = argv[++i];
		else if (i > 0 && strcmp(argv[i], "--help") == 0)
			show_help();
		else
			game_argv[game_argc++] = argv[i];
	}
	return dir != NULL && dir[0] != '\0' ? dir : NULL;
}

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif

/* The folder the program itself is in; for Ultima7.app, the folder holding the app. */
static bool exe_directory(char *out, size_t size)
{
	char *slash;

#ifdef _WIN32
	DWORD n = GetModuleFileNameA(NULL, out, (DWORD) size);

	if (n == 0 || n >= size)
		return false;
	slash = strrchr(out, '\\');
#elif defined(__APPLE__)
	uint32_t n = (uint32_t) size;

	char *bundle;

	if (_NSGetExecutablePath(out, &n) != 0)
		return false;
	/* ".../Ultima7.app/Contents/MacOS/Ultima7" becomes ".../Ultima7.app". */
	bundle = strstr(out, ".app/Contents/MacOS/");
	if (bundle != NULL)
		bundle[4] = '\0';
	slash = strrchr(out, '/');
#else
	ssize_t n = readlink("/proc/self/exe", out, size - 1);

	if (n <= 0)
		return false;
	out[n] = '\0';
	slash = strrchr(out, '/');
#endif
	if (slash == NULL)
		return false;
	*slash = '\0';
	return true;
}

/* Ultima7 in the user's Documents folder. */
static bool documents_directory(char *out, size_t size)
{
#ifdef _WIN32
	char docs[MAX_PATH];

	if (SHGetFolderPathA(NULL, CSIDL_PERSONAL, NULL, 0, docs) != S_OK)
		return false;
	return snprintf(out, size, "%s\\Ultima7", docs) < (int) size;
#else
	const char *home = getenv("HOME");

	if (home == NULL || home[0] == '\0')
		return false;
	return snprintf(out, size, "%s/Documents/Ultima7", home) < (int) size;
#endif
}

#ifdef _WIN32

/* The exe is a windowed program, so a double-click opens no console. Started from a terminal,
 * its messages go to that terminal; output already redirected to a file or pipe stays there. */
static void use_parent_console(void)
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
		return;
	if (GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_UNKNOWN)
		freopen("CONOUT$", "w", stdout);
	if (GetFileType(GetStdHandle(STD_ERROR_HANDLE)) == FILE_TYPE_UNKNOWN)
		freopen("CONOUT$", "w", stderr);
}
#else
static void use_parent_console(void)
{
}
#endif

int main(int argc, char **argv)
{
	MTY_Frame frame;
	char problem[1024], own_dir[1024] = "", docs_dir[1024];
	const char *data_dir;

	use_parent_console();
	on_app_thread = true;
#ifdef U7_RESET_CHECK
	reset_check_startup();
#endif
	start_time = MTY_GetTime();
	data_dir = take_data_dir(argc, argv);
	files_set_root(data_dir != NULL ? data_dir : ".");
	/* With no folder named: the current one, the program's own (a double-click does not always
	 * start in the game's folder), then Documents/Ultima7. Documents comes last because macOS
	 * asks the player before letting an app look there. */
	if (data_dir == NULL && !plat_dir_exists("STATIC")) {
		const char *fallback = exe_directory(own_dir, sizeof own_dir) ? own_dir : ".";

		files_set_root(fallback);
		if (!plat_dir_exists("STATIC") && documents_directory(docs_dir, sizeof docs_dir)) {
			files_set_root(docs_dir);
#ifndef __APPLE__
			/* Found nowhere: report the program's folder, where the game is meant to go. A Mac
			 * install keeps the app in Applications and the game in Documents/Ultima7. */
			if (!plat_dir_exists("STATIC"))
				files_set_root(fallback);
#endif
		}
	}
	if (!files_check_data(problem, sizeof problem)) {
		/* macOS runs a downloaded app from a hidden copy until it is moved in Finder. */
		if (strstr(own_dir, "/AppTranslocation/") != NULL)
			snprintf(problem, sizeof problem, "macOS started Ultima7 from a temporary copy, so "
				"it can't see the game files. In Finder, drag Ultima7.app out of the game "
				"folder and back in, then open it again.");
		plat_fatal(problem);
	}
	nullpage_install();
	events_init();
	video_init();
	view_lock = MTY_MutexCreate();
	program_done = MTY_WaitableCreate();

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
	MTY_ThreadDetach(launcher_thread, NULL);
	MTY_AppRun(app);

	MTY_AppDestroy(&app);
	end_process(MTY_Atomic32Get(&exit_code));
	return 0;
}
