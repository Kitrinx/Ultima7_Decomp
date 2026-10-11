/* Scripted input for testing, from the U7_TEST_INPUT environment variable:
 *
 *   U7_TEST_INPUT="4000 key i; 6000 move 320 100; 6100 down; 6600 move 400 120; 6700 up"
 *
 * Each step is a time in milliseconds since the process started and an action: "key" with a
 * character or esc, enter, space, up, down, left, right, f1 to f12; "move x y" in the game's
 * 640x200 mouse space; "down" and "up" for the left button, "rdown" and "rup" for the right;
 * "shot file.ppm" saves the frame on screen; "end n" makes the running program exit with code
 * n, as if it had quit by itself; "quit" ends the run. The clock runs across the whole session,
 * so under the launcher one script drives every program in turn; a step that comes due between
 * programs goes to the next. Steps go through the same event handling as real input. While a
 * script runs no window opens.
 *
 * Wall-clock steps land on different game frames from run to run. A script that starts with
 * "frames;" times each step in frames the game has presented instead, and one that starts with
 * "fixed;" in milliseconds; both run the game on a clock that moves only as the game waits, so
 * every run takes the same path. U7_TEST_CLOCK=frames or fixed does the same for any script:
 *
 *   U7_TEST_INPUT="frames; 300 key i; 420 shot inventory.ppm; 500 quit"
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

#ifdef _WIN32
#define strtok_r strtok_s
#endif

#define MAX_STEPS 256

typedef struct {
	int64_t at;	/* milliseconds, or frames in a frame-timed script */
	MTY_Event event;
	int32_t game_x, game_y;
	bool is_move, is_quit, is_end;
	int16_t end_code;
	char shot[128];
} step;

static step steps[MAX_STEPS];
static int step_count = -1;
static int next_step;

enum { CLOCK_WALL, CLOCK_FIXED, CLOCK_FRAMES };
static int clock_kind = -1;
static MTY_Atomic64 fixed_us;
static int64_t frames_shown, frame_shown_us;

/* Each pump of host events takes this long on the fixed clock, so loops that only poll still
 * see time pass. */
#define PUMP_US 50
/* While the game shows nothing new, as in a menu waiting for a key, the frame count still moves
 * on this often. */
#define IDLE_FRAME_US 100000

static int test_clock_kind(void)
{
	if (clock_kind < 0) {
		const char *script = getenv("U7_TEST_INPUT"), *chosen = getenv("U7_TEST_CLOCK");
		char word[8] = {0};

		if (script != NULL && sscanf(script, " %7[a-z]", word) == 1)
			chosen = word;
		clock_kind = script == NULL || chosen == NULL ? CLOCK_WALL
			: strcmp(chosen, "frames") == 0 ? CLOCK_FRAMES
			: strcmp(chosen, "fixed") == 0 ? CLOCK_FIXED : CLOCK_WALL;
	}
	return clock_kind;
}

bool test_clock_fixed(void)
{
	return test_clock_kind() != CLOCK_WALL;
}

int64_t test_clock_us(void)
{
	return MTY_Atomic64Get(&fixed_us);
}

void backend_sleep(uint32_t milliseconds)
{
	if (test_clock_fixed())
		MTY_Atomic64Add(&fixed_us, (int64_t) milliseconds * 1000);
	MTY_Sleep(milliseconds);
}

static MTY_Key named_key(const char *name)
{
	if (strcmp(name, "esc") == 0) return MTY_KEY_ESCAPE;
	if (strcmp(name, "enter") == 0) return MTY_KEY_ENTER;
	if (strcmp(name, "space") == 0) return MTY_KEY_SPACE;
	if (strcmp(name, "up") == 0) return MTY_KEY_UP;
	if (strcmp(name, "down") == 0) return MTY_KEY_DOWN;
	if (strcmp(name, "left") == 0) return MTY_KEY_LEFT;
	if (strcmp(name, "right") == 0) return MTY_KEY_RIGHT;
	if (name[0] == 'f' && name[1] >= '1' && name[1] <= '9') {
		static const MTY_Key function_keys[] = {MTY_KEY_F1, MTY_KEY_F2, MTY_KEY_F3, MTY_KEY_F4,
			MTY_KEY_F5, MTY_KEY_F6, MTY_KEY_F7, MTY_KEY_F8, MTY_KEY_F9, MTY_KEY_F10, MTY_KEY_F11,
			MTY_KEY_F12};
		int n = atoi(name + 1);

		return n >= 1 && n <= 12 ? function_keys[n - 1] : MTY_KEY_NONE;
	}
	return name[1] == 0 ? keys_for_char(name[0]) : MTY_KEY_NONE;
}

static void add_button(int64_t at, MTY_Button button, bool pressed)
{
	step *s = &steps[step_count++];

	s->at = at;
	s->event.type = MTY_EVENT_BUTTON;
	s->event.button.button = button;
	s->event.button.pressed = pressed;
}

static void parse(void)
{
	const char *text = getenv("U7_TEST_INPUT");
	char buffer[4096], *part, *cursor;

	step_count = 0;
	if (text == NULL)
		return;
	strncpy(buffer, text, sizeof buffer - 1);
	buffer[sizeof buffer - 1] = 0;
	for (part = strtok_r(buffer, ";", &cursor); part != NULL && step_count < MAX_STEPS - 1;
		part = strtok_r(NULL, ";", &cursor)) {
		long at;
		/* how long a key stays down */
		int64_t release = test_clock_kind() == CLOCK_FRAMES ? 3 : 50;
		char action[16] = {0}, arg[16] = {0};
		int x, y;

		if (sscanf(part, "%ld %15s", &at, action) != 2)
			continue;
		if (strcmp(action, "key") == 0 && sscanf(part, "%*d %*s %15s", arg) == 1) {
			MTY_Key key = named_key(arg);
			if (key == MTY_KEY_NONE)
				continue;
			for (int pressed = 1; pressed >= 0; pressed--) {
				step *s = &steps[step_count++];
				s->at = at + (pressed ? 0 : release);
				s->event.type = MTY_EVENT_KEY;
				s->event.key.key = key;
				s->event.key.pressed = pressed != 0;
			}
		} else if (strcmp(action, "move") == 0 && sscanf(part, "%*d %*s %d %d", &x, &y) == 2) {
			step *s = &steps[step_count++];
			s->at = at;
			s->is_move = true;
			s->game_x = x;
			s->game_y = y;
			s->event.type = MTY_EVENT_MOTION;
		} else if (strcmp(action, "shot") == 0 && sscanf(part, "%*d %*s %127s", steps[step_count].shot) == 1) {
			steps[step_count++].at = at;
		} else if (strcmp(action, "end") == 0 && sscanf(part, "%*d %*s %d", &x) == 1) {
			steps[step_count].at = at;
			steps[step_count].end_code = (int16_t) x;
			steps[step_count++].is_end = true;
		} else if (strcmp(action, "quit") == 0) {
			steps[step_count].at = at;
			steps[step_count++].is_quit = true;
		} else if (strcmp(action, "down") == 0) {
			add_button(at, MTY_BUTTON_LEFT, true);
		} else if (strcmp(action, "up") == 0) {
			add_button(at, MTY_BUTTON_LEFT, false);
		} else if (strcmp(action, "rdown") == 0) {
			add_button(at, MTY_BUTTON_RIGHT, true);
		} else if (strcmp(action, "rup") == 0) {
			add_button(at, MTY_BUTTON_RIGHT, false);
		}
	}
}

bool test_input_active(void)
{
	return getenv("U7_TEST_INPUT") != NULL;
}

/* Runs the steps that are due, or only those up to the first that is not a shot. */
static void run_due_steps(bool shots_only)
{
	int64_t now = test_clock_kind() == CLOCK_FRAMES ? frames_shown : backend_elapsed_us() / 1000;

	if (step_count < 0)
		parse();
	while (next_step < step_count && steps[next_step].at <= now
		&& (!shots_only || steps[next_step].shot[0] != 0)) {
		step *s = &steps[next_step++];
		MTY_Size view = backend_view_size();

		if (s->is_quit) {
			backend_request_exit(0);
		} else if (s->is_end) {
			plat_exit(s->end_code);
		} else if (s->shot[0] != 0) {
			video_write_shot(s->shot);
		} else {
			if (s->is_move) {
				int32_t vx, vy, vw, vh;

				/* The centre of the game's mouse position, in window pixels. */
				video_view_rect(view, &vx, &vy, &vw, &vh);
				s->event.motion.x = vx + (2 * s->game_x + 1) * vw / (2 * 640);
				s->event.motion.y = vy + (2 * s->game_y + 1) * vh / (2 * 200);
			}
			events_push(&s->event, view);
		}
	}
}

void test_input_poll(void)
{
	if (test_clock_fixed())
		MTY_Atomic64Add(&fixed_us, PUMP_US);
	while (test_clock_kind() == CLOCK_FRAMES
		&& test_clock_us() - frame_shown_us >= IDLE_FRAME_US) {
		frames_shown++;
		frame_shown_us += IDLE_FRAME_US;
	}
	run_due_steps(false);
}

void test_input_frame(void)
{
	frames_shown++;
	frame_shown_us = test_clock_us();
	if (test_clock_kind() == CLOCK_FRAMES)
		run_due_steps(true);
}
