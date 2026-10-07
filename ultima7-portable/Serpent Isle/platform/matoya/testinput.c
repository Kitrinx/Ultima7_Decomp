/* Scripted input for testing, from the U7_TEST_INPUT environment variable:
 *
 *   U7_TEST_INPUT="4000 key i; 6000 move 320 100; 6100 down; 6600 move 400 120; 6700 up"
 *
 * Each step is a time in milliseconds since the process started and an action: "key" with a
 * character or esc, enter, space, up, down, left, right, f1 to f12, alt- before any of them for
 * the Alt key; "move x y" in the game's
 * 640x200 mouse space; "down" and "up" for the left button, "rdown" and "rup" for the right;
 * "shot file.ppm" saves the frame on screen; "end n" makes the running program exit with code
 * n, as if it had quit by itself; "quit" ends the run. The clock runs across the whole session,
 * so under the launcher one script drives every program in turn; a step that comes due between
 * programs goes to the next. Steps go through the same event handling as real input. While a
 * script runs the window stays hidden and never takes focus.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

#ifdef _WIN32
#define strtok_r strtok_s
#endif

#define MAX_STEPS 4096

typedef struct {
	int64_t at_ms;
	MTY_Event event;
	int32_t game_x, game_y;
	bool is_move, is_quit, is_end;
	int16_t end_code;
	char shot[128];
} step;

static step steps[MAX_STEPS];
static int step_count = -1;
static int next_step;

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

	s->at_ms = at;
	s->event.type = MTY_EVENT_BUTTON;
	s->event.button.button = button;
	s->event.button.pressed = pressed;
}

static void parse(void)
{
	const char *text = getenv("U7_TEST_INPUT");
	static char buffer[65536];
	char *part, *cursor;

	step_count = 0;
	if (text == NULL)
		return;
	strncpy(buffer, text, sizeof buffer - 1);
	buffer[sizeof buffer - 1] = 0;
	for (part = strtok_r(buffer, ";", &cursor); part != NULL && step_count < MAX_STEPS - 1;
		part = strtok_r(NULL, ";", &cursor)) {
		long at;
		char action[16] = {0}, arg[16] = {0};
		int x, y;

		if (sscanf(part, "%ld %15s", &at, action) != 2)
			continue;
		if (strcmp(action, "key") == 0 && sscanf(part, "%*d %*s %15s", arg) == 1) {
			bool alt = strncmp(arg, "alt-", 4) == 0;
			MTY_Key key = named_key(alt ? arg + 4 : arg);
			if (key == MTY_KEY_NONE)
				continue;
			for (int pressed = 1; pressed >= 0; pressed--) {
				step *s = &steps[step_count++];
				s->at_ms = at + (pressed ? 0 : 50);
				s->event.type = MTY_EVENT_KEY;
				s->event.key.key = key;
				s->event.key.pressed = pressed != 0;
				s->event.key.mod = alt ? MTY_MOD_LALT : MTY_MOD_NONE;
			}
		} else if (strcmp(action, "move") == 0 && sscanf(part, "%*d %*s %d %d", &x, &y) == 2) {
			step *s = &steps[step_count++];
			s->at_ms = at;
			s->is_move = true;
			s->game_x = x;
			s->game_y = y;
			s->event.type = MTY_EVENT_MOTION;
		} else if (strcmp(action, "shot") == 0 && sscanf(part, "%*d %*s %127s", steps[step_count].shot) == 1) {
			steps[step_count++].at_ms = at;
		} else if (strcmp(action, "end") == 0 && sscanf(part, "%*d %*s %d", &x) == 1) {
			steps[step_count].at_ms = at;
			steps[step_count].end_code = (int16_t) x;
			steps[step_count++].is_end = true;
		} else if (strcmp(action, "quit") == 0) {
			steps[step_count].at_ms = at;
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

void test_input_poll(void)
{
	int64_t now_ms = backend_elapsed_us() / 1000;

	if (step_count < 0)
		parse();
	while (next_step < step_count && steps[next_step].at_ms <= now_ms) {
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
