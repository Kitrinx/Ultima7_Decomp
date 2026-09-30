/* Keyboard, mouse, time and game timers.
 *
 * The app thread queues window events; plat_pump drains them on the game thread, so the
 * key buffer, modifier flags, mouse state and mouse handler are only touched there.
 */
#include <string.h>

#include "backend.h"

#define EVENT_QUEUE_SIZE 256
#define KEY_BUFFER_SIZE 64
#define MAX_TIMERS 16
#define MAX_CATCH_UP_TICKS 5
#define MOUSE_WIDTH 640
#define MOUSE_HEIGHT 200

enum { EVENT_KEY, EVENT_MOVE, EVENT_BUTTON, EVENT_FOCUS_LOST };

typedef struct {
	uint8_t kind;
	bool pressed;
	uint16_t key;
	uint16_t mod;
	uint16_t button;
	int16_t x;
	int16_t y;
} queued_event;

/* Shared with the app thread, under queue_lock. */
static MTY_Mutex *queue_lock;
static queued_event queue[EVENT_QUEUE_SIZE];
static uint32_t queue_count;
static bool warp_pending;
static int16_t warp_x;
static int16_t warp_y;

/* Game thread only. */
static uint16_t key_buffer[KEY_BUFFER_SIZE];
static uint32_t key_head;
static uint32_t key_count;
static int16_t pending_scan = -1;
static uint16_t modifiers;
static int16_t mouse_x;
static int16_t mouse_y;
static uint16_t mouse_buttons;
static plat_mouse_fn mouse_handler;
static plat_timer_fn timers[MAX_TIMERS];
static bool timers_started;
static uint32_t timer_ticks;
static bool pumping;

void events_init(void)
{
	queue_lock = MTY_MutexCreate();
}

/* ---- Keys ---- */

/* US layout by scan code 0x00-0x39; 0 where the key gives no character. */
static const char plain_chars[] =
	"\0\033" "1234567890-=\b\t"
	"qwertyuiop[]\r\0"
	"asdfghjkl;'`\0\\"
	"zxcvbnm,./\0*\0 ";
static const char shifted_chars[] =
	"\0\033" "!@#$%^&*()_+\b\t"
	"QWERTYUIOP{}\r\0"
	"ASDFGHJKL:\"~\0|"
	"ZXCVBNM<>?\0*\0 ";
/* Keypad scan codes 0x47-0x53 with Num Lock on. */
static const char keypad_chars[] = "789-456+1230.";

static uint16_t word(uint8_t scan, uint8_t ascii)
{
	return (uint16_t) (scan << 8 | ascii);
}

/* Ctrl with Home, arrows, PgUp and the rest; 0 where BIOS gave nothing. */
static uint8_t ctrl_navigation(uint8_t scan)
{
	switch (scan) {
	case 0x47: return 0x77;
	case 0x48: return 0x8D;
	case 0x49: return 0x84;
	case 0x4B: return 0x73;
	case 0x4D: return 0x74;
	case 0x4F: return 0x75;
	case 0x50: return 0x91;
	case 0x51: return 0x76;
	case 0x52: return 0x92;
	case 0x53: return 0x93;
	default: return 0;
	}
}

static uint16_t navigation_word(uint8_t scan, bool ctrl, bool alt)
{
	if (alt)
		return word(scan + 0x50, 0);
	if (ctrl)
		return word(ctrl_navigation(scan), 0);
	return word(scan, 0);
}

static uint16_t main_block_word(uint8_t scan, bool shift, bool ctrl, bool alt, bool caps)
{
	char c = plain_chars[scan];
	bool letter = c >= 'a' && c <= 'z';

	if (alt) {
		if (letter)
			return word(scan, 0);
		/* Alt-1 .. Alt-= are 0x78 .. 0x83. */
		if (scan >= 0x02 && scan <= 0x0D)
			return word(scan + 0x76, 0);
		return c == ' ' ? word(scan, ' ') : 0;
	}
	if (ctrl) {
		if (letter)
			return word(scan, (uint8_t) (c - 'a' + 1));
		switch (c) {
		case '\033': return word(scan, 27);
		case '[': return word(scan, 0x1B);
		case '\\': return word(scan, 0x1C);
		case ']': return word(scan, 0x1D);
		case '6': return word(scan, 0x1E);
		case '-': return word(scan, 0x1F);
		case '2': return word(0x03, 0);
		case '\r': return word(scan, '\n');
		case '\b': return word(scan, 0x7F);
		case ' ': return word(scan, ' ');
		default: return 0;
		}
	}
	if (c == '\t' && shift)
		return word(scan, 0);
	if (letter && caps)
		shift = !shift;
	return word(scan, (uint8_t) (shift ? shifted_chars[scan] : c));
}

static uint16_t function_key_word(uint8_t n, bool shift, bool ctrl, bool alt)
{
	if (n < 10) {
		if (alt)
			return word(0x68 + n, 0);
		if (ctrl)
			return word(0x5E + n, 0);
		if (shift)
			return word(0x54 + n, 0);
		return word(0x3B + n, 0);
	}
	/* F11 and F12. */
	n -= 10;
	if (alt)
		return word(0x8B + n, 0);
	if (ctrl)
		return word(0x89 + n, 0);
	if (shift)
		return word(0x87 + n, 0);
	return word(0x85 + n, 0);
}

uint16_t keys_bios_word(MTY_Key key, MTY_Mod mod)
{
	/* libmatoya key codes are set 1 scan codes, with 0x100 marking the E0-prefixed keys. */
	uint8_t scan = (uint8_t) (key & 0xFF);
	bool extended = (key & 0x100) != 0;
	bool shift = (mod & MTY_MOD_SHIFT) != 0;
	bool ctrl = (mod & MTY_MOD_CTRL) != 0;
	bool alt = (mod & MTY_MOD_ALT) != 0;
	bool caps = (mod & MTY_MOD_CAPS) != 0;
#ifdef __APPLE__
	/* Mac keyboards have no Num Lock; their keypads always type digits. */
	bool num = true;
#else
	bool num = (mod & MTY_MOD_NUM) != 0;
#endif

	if (extended) {
		switch (key) {
		case MTY_KEY_NP_ENTER: return word(0x1C, ctrl ? '\n' : '\r');
		case MTY_KEY_NP_DIVIDE: return alt ? 0 : word(0x35, '/');
		case MTY_KEY_HOME: case MTY_KEY_UP: case MTY_KEY_PAGE_UP:
		case MTY_KEY_LEFT: case MTY_KEY_RIGHT:
		case MTY_KEY_END: case MTY_KEY_DOWN: case MTY_KEY_PAGE_DOWN:
		case MTY_KEY_INSERT: case MTY_KEY_DELETE:
			return navigation_word(scan, ctrl, alt);
		default: return 0;
		}
	}
	if (scan >= 0x3B && scan <= 0x44)
		return function_key_word(scan - 0x3B, shift, ctrl, alt);
	if (scan == 0x57 || scan == 0x58)
		return function_key_word(scan - 0x57 + 10, shift, ctrl, alt);
	if (scan >= 0x47 && scan <= 0x53) {
		char c = keypad_chars[scan - 0x47];

		if (c == '-' || c == '+')
			return alt ? 0 : word(scan, (uint8_t) c);
		/* Shift flips the keypad between digits and movement, as Num Lock does. */
		if (num != shift && !ctrl && !alt)
			return word(scan, (uint8_t) c);
		return scan == 0x4C ? 0 : navigation_word(scan, ctrl, alt);
	}
	if (key == MTY_KEY_NP_EQUAL)
		return word(0x0D, '=');
	if (scan < sizeof plain_chars - 1 && plain_chars[scan] != 0)
		return main_block_word(scan, shift, ctrl, alt, caps);
	return 0;
}

static uint16_t bios_modifiers(MTY_Mod mod)
{
	uint16_t flags = 0;

	if (mod & MTY_MOD_RSHIFT)
		flags |= 1;
	if (mod & MTY_MOD_LSHIFT)
		flags |= 2;
	if (mod & MTY_MOD_CTRL)
		flags |= 4;
	if (mod & MTY_MOD_ALT)
		flags |= 8;
	return flags;
}

int16_t plat_key_available(void)
{
	if (pending_scan < 0 && key_count == 0)
		plat_pump();
	return pending_scan >= 0 || key_count > 0;
}

int16_t plat_key_get(void)
{
	uint16_t next;

	while (!plat_key_available())
		plat_yield();
	if (pending_scan >= 0) {
		next = (uint16_t) pending_scan;
		pending_scan = -1;
		return (int16_t) next;
	}
	next = key_buffer[key_head];
	key_head = (key_head + 1) % KEY_BUFFER_SIZE;
	key_count--;
	if ((next & 0xFF) == 0) {
		pending_scan = (int16_t) (next >> 8);
		return 0;
	}
	return (int16_t) (next & 0xFF);
}

uint16_t plat_key_modifiers(void)
{
	return modifiers;
}

/* ---- Mouse ---- */

static int16_t clamp16(int32_t v, int32_t low, int32_t high)
{
	return (int16_t) (v < low ? low : v > high ? high : v);
}

void plat_mouse_state(int16_t *x, int16_t *y, uint16_t *buttons)
{
	if (x != NULL)
		*x = mouse_x;
	if (y != NULL)
		*y = mouse_y;
	if (buttons != NULL)
		*buttons = mouse_buttons;
}

void plat_mouse_set_handler(plat_mouse_fn fn)
{
	mouse_handler = fn;
}

void plat_mouse_move_to(int16_t x, int16_t y)
{
	mouse_x = clamp16(x, 0, MOUSE_WIDTH - 1);
	mouse_y = clamp16(y, 0, MOUSE_HEIGHT - 1);
	/* The game's static constructors call this before main() creates the lock. */
	if (queue_lock == NULL)
		return;
	MTY_MutexLock(queue_lock);
	warp_pending = true;
	warp_x = mouse_x;
	warp_y = mouse_y;
	MTY_MutexUnlock(queue_lock);
}

bool events_take_warp(MTY_Size view, uint32_t *x, uint32_t *y)
{
	int32_t vx, vy, vw, vh;
	bool pending;

	MTY_MutexLock(queue_lock);
	pending = warp_pending;
	warp_pending = false;
	video_view_rect(view, &vx, &vy, &vw, &vh);
	/* The centre of the target mouse position. */
	*x = (uint32_t) (vx + (2 * warp_x + 1) * vw / (2 * MOUSE_WIDTH));
	*y = (uint32_t) (vy + (2 * warp_y + 1) * vh / (2 * MOUSE_HEIGHT));
	MTY_MutexUnlock(queue_lock);
	return pending;
}

/* ---- Queue (app thread side) ---- */

static void map_point(MTY_Size view, int32_t wx, int32_t wy, queued_event *out)
{
	int32_t vx, vy, vw, vh;

	video_view_rect(view, &vx, &vy, &vw, &vh);
	if (vw <= 0 || vh <= 0)
		return;
	out->x = clamp16((int32_t) ((int64_t) (wx - vx) * MOUSE_WIDTH / vw), 0, MOUSE_WIDTH - 1);
	out->y = clamp16((int32_t) ((int64_t) (wy - vy) * MOUSE_HEIGHT / vh), 0, MOUSE_HEIGHT - 1);
}

void events_push(const MTY_Event *event, MTY_Size view)
{
	queued_event e = {0};

	switch (event->type) {
	case MTY_EVENT_KEY:
		e.kind = EVENT_KEY;
		e.key = (uint16_t) event->key.key;
		e.mod = (uint16_t) event->key.mod;
		e.pressed = event->key.pressed;
		break;
	case MTY_EVENT_MOTION:
		if (event->motion.relative)
			return;
		e.kind = EVENT_MOVE;
		map_point(view, event->motion.x, event->motion.y, &e);
		break;
	case MTY_EVENT_BUTTON:
		if (event->button.button == MTY_BUTTON_LEFT)
			e.button = 1;
		else if (event->button.button == MTY_BUTTON_RIGHT)
			e.button = 2;
		else
			return;
		/* Positions come from motion events only: libmatoya sends releases it notices late
		 * without a position, which would read as the window's corner. */
		e.kind = EVENT_BUTTON;
		e.pressed = event->button.pressed;
		break;
	case MTY_EVENT_FOCUS:
		if (event->focus)
			return;
		e.kind = EVENT_FOCUS_LOST;
		break;
	default:
		return;
	}

	MTY_MutexLock(queue_lock);
	/* Only the latest of a run of moves matters. */
	if (e.kind == EVENT_MOVE && queue_count > 0 && queue[queue_count - 1].kind == EVENT_MOVE)
		queue[queue_count - 1] = e;
	else if (queue_count < EVENT_QUEUE_SIZE)
		queue[queue_count++] = e;
	MTY_MutexUnlock(queue_lock);
}

/* ---- Pump (game thread) ---- */

static void deliver_mouse(uint16_t events)
{
	if (mouse_handler != NULL)
		mouse_handler(events, mouse_buttons, mouse_x, mouse_y);
}

static void handle(const queued_event *e)
{
	uint16_t bios;

	switch (e->kind) {
	case EVENT_KEY:
		modifiers = bios_modifiers((MTY_Mod) e->mod);
		if (!e->pressed)
			break;
		bios = keys_bios_word((MTY_Key) e->key, (MTY_Mod) e->mod);
		/* A full buffer drops the key, as the BIOS did. */
		if (bios != 0 && key_count < KEY_BUFFER_SIZE) {
			key_buffer[(key_head + key_count) % KEY_BUFFER_SIZE] = bios;
			key_count++;
		}
		break;
	case EVENT_MOVE:
		if (e->x == mouse_x && e->y == mouse_y)
			break;
		mouse_x = e->x;
		mouse_y = e->y;
		deliver_mouse(PLAT_MOUSE_MOVED);
		break;
	case EVENT_BUTTON:
		if (e->pressed == ((mouse_buttons & e->button) != 0))
			break;
		mouse_buttons ^= e->button;
		if (e->button == 1)
			deliver_mouse(e->pressed ? PLAT_MOUSE_LEFT_DOWN : PLAT_MOUSE_LEFT_UP);
		else
			deliver_mouse(e->pressed ? PLAT_MOUSE_RIGHT_DOWN : PLAT_MOUSE_RIGHT_UP);
		break;
	case EVENT_FOCUS_LOST:
		modifiers = 0;
		break;
	}
}

static void run_timers(void)
{
	uint32_t now = plat_ticks();
	plat_timer_fn due[MAX_TIMERS];

	if (!timers_started) {
		timers_started = true;
		timer_ticks = now;
	}
	if (now - timer_ticks > MAX_CATCH_UP_TICKS)
		timer_ticks = now - MAX_CATCH_UP_TICKS;
	while (timer_ticks != now) {
		timer_ticks++;
		/* A timer may add or remove timers. */
		memcpy(due, timers, sizeof due);
		for (int i = 0; i < MAX_TIMERS; i++) {
			if (due[i] != NULL)
				due[i]();
		}
	}
}

void plat_pump(void)
{
	queued_event taken[EVENT_QUEUE_SIZE];
	uint32_t count;

	/* A timer or mouse handler that polls the keyboard must not pump again. */
	if (pumping)
		return;
	pumping = true;
	test_input_poll();

	MTY_MutexLock(queue_lock);
	count = queue_count;
	memcpy(taken, queue, count * sizeof *queue);
	queue_count = 0;
	MTY_MutexUnlock(queue_lock);

	for (uint32_t i = 0; i < count; i++)
		handle(&taken[i]);
	run_timers();
	/* VGA memory was always on screen, and the pointer is drawn into it as the mouse moves,
	 * so loops that only poll for input still show it. */
	video_refresh(false);
	pumping = false;
}

/* ---- Time ---- */

uint32_t plat_ticks(void)
{
	return (uint32_t) (backend_elapsed_us() * PLAT_TICKS_PER_SECOND / 1000000);
}

uint32_t plat_milliseconds(void)
{
	return (uint32_t) (backend_elapsed_us() / 1000);
}

void plat_sleep(uint32_t milliseconds)
{
	MTY_Sleep(milliseconds);
}

void plat_yield(void)
{
	plat_pump();
	video_refresh(false);
	MTY_Sleep(1);
}

void plat_game_timer_add(plat_timer_fn fn)
{
	int free_slot = -1;

	for (int i = 0; i < MAX_TIMERS; i++) {
		if (timers[i] == fn)
			return;
		if (timers[i] == NULL && free_slot < 0)
			free_slot = i;
	}
	if (free_slot < 0)
		plat_fatal("Too many game timers.");
	timers[free_slot] = fn;
}

void plat_game_timer_remove(plat_timer_fn fn)
{
	for (int i = 0; i < MAX_TIMERS; i++) {
		if (timers[i] == fn)
			timers[i] = NULL;
	}
}

MTY_Key keys_for_char(char c)
{
	for (uint32_t scan = 1; scan < sizeof plain_chars - 1; scan++) {
		if (plain_chars[scan] == c)
			return (MTY_Key) scan;
	}
	return MTY_KEY_NONE;
}
