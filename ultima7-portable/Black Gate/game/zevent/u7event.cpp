/* Black Gate U7.EXE, resident segment 56 (file offsets 0x0213e8 to 0x021cf2, 2314 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "plat.h"
#include "dosio.h"
#include "u7point.h"
#include "look.h"
#include "partymov.h"
#include "mainctrl.h"
#include "msclick.h"
#include "mouse.h"
#include "systimer.h"
#include "u7event.h"

#define KEY_ESCAPE          27
#define NO_DIRECTION        16
#define MOUSE_QUEUE_SIZE    10

extern MouseState MouseEventQueue[MOUSE_QUEUE_SIZE];

inline uint8_t CtrlReleased()
{
	return !CtrlStatus && PreviousCtrlStatus != 0;
}
inline uint8_t CtrlPressed()
{
	return CtrlStatus != 0 && !PreviousCtrlStatus;
}

Input::Input() : queue((char *)MouseEventQueue, MOUSE_QUEUE_SIZE)
{
	DoubleClickDelay = 12;
}

void Input::initialize()
{
	InitMouse(0);
	queue.install();
	LoadPointerShapes(this, 148, 88);
	InstallCursorHook(this);
	GumpMode = 0;
	KeyMouseEnabled = 0;
	KeyMouseX = 320;
	KeyMouseY = 100;
	PreviousCtrlStatus = CtrlStatus = 0;
	KeyMouseReleasePending = 0;
}

void RunKeyboardMouse(void)
{
	uint8_t oldCursor;
	MouseState state;

	oldCursor = CursorBase;
	SelectMouseCursor(0);
	while (KeyMouseEnabled != 0) {
		plat_yield();
		UpdateAndCopyMouseState(&state);
		if (state.action == MOUSE_CLICK) {
			LookAtItem();
			KeyMouseEnabled = 0;
		} else if (state.action == MOUSE_DOUBLE_CLICK) {
			UseAtPointer(MouseState_getX(&state), state.y);
			KeyMouseEnabled = 0;
		}
	}
	SelectMouseCursor(oldCursor);
	if (MousePresent != 0)
		ShowCursor();
}

void PollKeyAndTranslateWithMouse(uint16_t *key, int16_t *x, int16_t *y)
{
	int16_t shift;
	uint8_t event, button;
	/* Arrow keys skip the mouse read below; the original then read whatever was on the stack. */
	MouseState state = {};

	*key = 0;
	*x = *y = 0;
	*key = PollKey(1);
	shift = GetLeftAndRightShiftStatus();
	switch (*key) {
	case ' ':
		KeyMouseX = MouseX;
		KeyMouseY = MouseY;
		KeyMouseEnabled = 1;
		RunKeyboardMouse();
		*key = 0;
		return;
	case KEY_UP:
		if (shift != 0)
			*key = '8';
		break;
	case KEY_DOWN:
		if (shift != 0)
			*key = '2';
		break;
	case KEY_LEFT:
		if (shift != 0)
			*key = '4';
		break;
	case KEY_RIGHT:
		if (shift != 0)
			*key = '6';
		break;
	default:
		UpdateAndCopyMouseState(&state);
	}
	event = state.action;
	button = state.button;
	if (state.valid()) {
		if (event == 3 && button == HeldMouseButton) {
			*key = button + KEY_MOUSE_RELEASE;
			HeldMouseButton = 0;
		} else if (event == 2) {
			*key = button + KEY_MOUSE_DOUBLE;
			if (!HeldMouseButton)
				HeldMouseButton = state.button;
		} else if (event == 1) {
			*key = button + KEY_MOUSE_CLICK;
			if (!HeldMouseButton)
				HeldMouseButton = button;
		} else if (HeldMouseButton != 0) {
			*key = button + KEY_MOUSE_HELD;
		}
	} else if (HeldMouseButton != 0) {
		*key = button + KEY_MOUSE_HELD;
	}
	*x = MouseState_getX(&state);
	*y = state.y;
}

static int16_t PushedBackKey = -1;

/* Nonzero when a key is waiting; handles the host's events first. */
int16_t KeyPressed(void)
{
	plat_pump();
	return PushedBackKey != -1 || plat_key_available();
}

/* The next key, waiting for one: ASCII, or 0 and then the scan code. */
int16_t ReadKey(void)
{
	int16_t key;

	if (PushedBackKey != -1) {
		key = PushedBackKey;
		PushedBackKey = -1;
		return key;
	}
	while (!KeyPressed())
		plat_yield();
	return plat_key_get();
}

/* Puts one key back, to be read again next. */
void UnreadKey(int16_t key)
{
	PushedBackKey = key;
}

void FlushKeyboard(void)
{
	while (KeyPressed() != 0)
		ReadKey();
}

uint16_t PollKey(int16_t discard)
{
	uint16_t key = 0;

	if (KeyPressed() != 0) {
		key = ReadKey();
		if (key == 0)
			key = ReadKey() + 0x100;
		if (discard != 0) {
			while (KeyPressed() != 0)
				ReadKey();
		}
	}
	return key;
}

uint8_t PollKeyToGlobalDiscarding(void)
{
	if (KeyPressed() != 0) {
		PolledKey = PollKey(1);
		return 1;
	}
	return 0;
}

uint8_t PollEscapeKey(void)
{
	int16_t key;

	if (KeyPressed() != 0) {
		key = ReadKey();
		if (key == KEY_ESCAPE)
			return 1;
		UnreadKey(key);
		return 0;
	}
	/* no key: the original returned kbhit's 0 left in AX */
	return 0;
}

/* Reads the Ctrl key from the BIOS shift flags, keeping the last reading. */
void GetCtrlStatus(void)
{
	PreviousCtrlStatus = CtrlStatus;
	CtrlStatus = plat_key_modifiers() & 4;
}

/* The two Shift key bits of the BIOS shift flags. */
int16_t GetLeftAndRightShiftStatus(void)
{
	return plat_key_modifiers() & 3;
}

void SetKeyMouseEvent(uint8_t event)
{
	MouseEventQueue[0].action = event;
	MouseEventQueue[0].button = 1;
	MouseEventQueue[0].x = KeyMouseX;
	MouseEventQueue[0].y = KeyMouseY;
}

MouseState *PollKeyboardMouse(void)
{
	uint8_t shift;
	uint8_t moved = 0;
	uint16_t key;
	uint8_t direction;
	Timer clickTimer(INT32_C(12));

	if (MouseX != KeyMouseX || MouseY != KeyMouseY) {
		KeyMouseEnabled = 0;
		SetKeyMouseEvent(4);
	}
	if (KeyMouseReleasePending != 0) {
		SetKeyMouseEvent(3);
		KeyMouseReleasePending = 0;
		return MouseEventQueue;
	}
	key = PollKey(1);
	switch (key) {
	case '8':
	case KEY_UP:
	case KEY_CTRL_UP:
		direction = 0;
		break;
	case '9':
	case KEY_PAGE_UP:
	case KEY_CTRL_PAGE_UP:
		direction = 1;
		break;
	case '6':
	case KEY_RIGHT:
	case KEY_CTRL_RIGHT:
		direction = 2;
		break;
	case '3':
	case KEY_PAGE_DOWN:
	case KEY_CTRL_PAGE_DOWN:
		direction = 3;
		break;
	case '2':
	case KEY_DOWN:
	case KEY_CTRL_DOWN:
		direction = 4;
		break;
	case '1':
	case KEY_END:
	case KEY_CTRL_END:
		direction = 5;
		break;
	case '4':
	case KEY_LEFT:
	case KEY_CTRL_LEFT:
		direction = 6;
		break;
	case '7':
	case KEY_HOME:
	case KEY_CTRL_HOME:
		direction = 7;
		break;
	default:
		direction = NO_DIRECTION;
	}
	shift = GetLeftAndRightShiftStatus();
	if (direction != NO_DIRECTION) {
		if (shift != 0) {
			KeyMouseX += DirectionDX[direction] * 16;
			KeyMouseY += DirectionDY[direction] * 8;
		} else {
			KeyMouseX += DirectionDX[direction];
			KeyMouseY += DirectionDY[direction];
		}
		if (KeyMouseX < 0)
			KeyMouseX = 0;
		if (KeyMouseX > 639)
			KeyMouseX = 639;
		if (KeyMouseY < 0)
			KeyMouseY = 0;
		if (KeyMouseY > 199)
			KeyMouseY = 199;
		MouseX = KeyMouseX;
		MouseY = KeyMouseY;
		MoveCursorHook(0, 0, KeyMouseX, KeyMouseY);
		moved = 1;
	}
	Timer_restart(&clickTimer);
	GetCtrlStatus();
	if (CtrlReleased()) {
		SetKeyMouseEvent(3);
	} else if (CtrlPressed()) {
		while (!Timer_hasFinished(&clickTimer)) {
			plat_yield();
			GetCtrlStatus();
			if (CtrlReleased()) {
				Timer_set(&clickTimer, INT32_C(12));
				while (!Timer_hasFinished(&clickTimer)) {
					plat_yield();
					GetCtrlStatus();
					if (CtrlPressed()) {
						SetKeyMouseEvent(2);
						return MouseEventQueue;
					}
				}
				SetKeyMouseEvent(1);
				KeyMouseReleasePending = 1;
				return MouseEventQueue;
			}
		}
		SetKeyMouseEvent(1);
	} else {
		SetKeyMouseEvent(moved != 0 ? 4 : 0);
	}
	return MouseEventQueue;
}

MouseState *GetLastMouseState(void)
{
	if (KeyMouseEnabled != 0)
		return MouseEventQueue;
	else
		return GetDriverMouseState();
}

MouseState *UpdateAndGetMouseState(void)
{
	if (MousePresent && HeldMouseButton && !KeyMouseEnabled) {
		if (ReadMouseButtons() == 0)
			HeldMouseButton = 0;
	}
	if (KeyMouseEnabled) {
		return PollKeyboardMouse();
	} else {
		if (MouseStackOverrun)
			ReportError(0x6e03);
		GetMouseAction();
		return GetLastMouseState();
	}
}

MouseState *UpdateAndCopyMouseState(MouseState *state)
{
	if (KeyMouseEnabled != 0) {
		PollKeyboardMouse();
		*state = MouseEventQueue[0];
	} else {
		CopyMouseAction(state);
	}
	return state;
}

uint8_t PollPointerMoved(void)
{
	uint8_t button;

	button = CheckPointerMoved();
	return button;
}

void Input::enterGumpMode()
{
	GumpMode = 1;
}

void Input::leaveGumpMode()
{
	GumpMode = 0;
	HeldMouseButton = 0;
}

int16_t MouseState_getX(MouseState *state)
{
	return state->x >> 1;
}

MouseState *GetDriverMouseState(void)
{
	return &LastMouseAction;
}

int16_t GetPolledKey(void)
{
	return PolledKey;
}

uint8_t Input::isButtonPressed(uint8_t button)
{
	MouseState *state;

	UpdateAndGetMouseState();
	state = GetLastMouseState();
	return (state->action == MOUSE_CLICK || state->action == MOUSE_DOUBLE_CLICK) && state->button == button;
}

uint8_t Input::isButtonReleased(uint8_t button)
{
	MouseState *state;

	UpdateAndGetMouseState();
	state = GetLastMouseState();
	return state->action == MOUSE_RELEASE && state->button == button;
}

uint8_t Input::isGumpMode()
{
	return GumpMode == 1;
}

void SetMouseCursorPosition(int16_t x, int16_t y)
{
	if (KeyMouseEnabled != 0) {
		KeyMouseX = x << 1;
		KeyMouseY = y;
	}
	SetMousePosition(x << 1, y);
}

uint8_t InGumpMode(void)
{
	return GameInput.isGumpMode();
}

uint8_t MouseExists(void)
{
	return MousePresent;
}

void WaitForClickOrKey(void)
{
	while (!UpdateAndGetMouseState()->pressed(1) && !PollKeyToGlobalDiscarding())
		plat_yield();
}

void SelectNoArrowCursor(void)
{
	SelectMouseCursor(48);
}

void SelectArrowCursor(void)
{
	SelectMouseCursor(8);
}

void Input::enableKeyboardMouse()
{
	KeyMouseEnabled = 1;
	if (!MousePresent)
		SetMouseCursorPosition(KeyMouseX >> 1, KeyMouseY);
}

Input GameInput;
int16_t PolledKey = 0;
uint8_t HeldMouseButton = 0;
MouseState MouseEventQueue[MOUSE_QUEUE_SIZE];
uint8_t GumpMode = 0, KeyMouseEnabled = 0;
int16_t KeyMouseX = 0, KeyMouseY = 0;
uint8_t PreviousCtrlStatus = 0, CtrlStatus = 0, KeyMouseReleasePending = 0;
