/* Serpent Isle SI.EXE, resident segment 114 (file offsets 0x03b8d7 to 0x03bc5d, 902 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: mouse.c */
#include "u7port.h"
#include "plat.h"
#include "init.h"
#include "mouse.h"

uint8_t MousePresent = 0;
int16_t MouseButtonCount = 0;
int8_t MouseCallbackOff = 0;
int16_t MouseHandlerCount = 0;
int16_t MouseX;
int16_t MouseY;
MouseFunc MouseHandlers[MOUSE_HANDLER_SLOTS];
int16_t MouseHandlerMasks[MOUSE_HANDLER_SLOTS];
int16_t MouseStackOverrun = 0;

/* The pointer's allowed range, in driver units, as the driver kept it. */
static int16_t MouseXMin = 0, MouseXMax = 639;
static int16_t MouseYMin = 0, MouseYMax = 199;
static uint8_t MouseDispatching = 0;

#define MOUSE_ERROR(line)   AssertFail(__FILE__, line)

/* The driver counts two columns per pixel, so (320, 100) is the middle of the screen. */
Mouse::Mouse()
{
	InitMouse(0);
	SetMousePosition(320, 100);
	MouseX = 320;
	MouseY = 100;
}

void InitMouse(int8_t noCallback)
{
	ResetMouseDriver();
	MousePresent = 1;
	MouseButtonCount = 2;
	MouseCallbackOff = noCallback;
}

void ResetMouseDriver(void)
{
	MouseXMin = 0;
	MouseXMax = 639;
	MouseYMin = 0;
	MouseYMax = 199;
}

int16_t ReadMouseButtons(void)
{
	int16_t x, y;
	uint16_t buttons;

	plat_mouse_state(&x, &y, &buttons);
	return buttons;
}

static int16_t Clamp(int16_t value, int16_t lo, int16_t hi)
{
	if (value < lo)
		return lo;
	if (value > hi)
		return hi;
	return value;
}

void SetMousePosition(int16_t x, int16_t y)
{
	plat_mouse_move_to(Clamp(x, MouseXMin, MouseXMax), Clamp(y, MouseYMin, MouseYMax));
	MouseX = x;
	MouseY = y;
}

void SetMouseXRange(int16_t lo, int16_t hi)
{
	MouseXMin = lo;
	MouseXMax = hi;
}

void SetMouseYRange(int16_t lo, int16_t hi)
{
	MouseYMin = lo;
	MouseYMax = hi;
}

void SetMouseMickeys(int16_t h, int16_t v)
{
}

int16_t GetMouseX(void)
{
	return MouseX;
}

int16_t GetMouseY(void)
{
	return MouseY;
}

void PinMouseHere(void)
{
	int16_t x, y;
	uint16_t buttons;

	plat_mouse_state(&x, &y, &buttons);
	PinMouseAt(Clamp(x, MouseXMin, MouseXMax) >> 1, Clamp(y, MouseYMin, MouseYMax));
}

/* Pins the pointer at (x, y); the driver counts two columns per pixel. */
void PinMouseAt(int16_t x, int16_t y)
{
	x <<= 1;
	/* Held in the game only: moving the host pointer here and back made it jump on release. */
	MouseX = x;
	MouseY = y;
	SetMouseXRange(x, x);
	SetMouseYRange(y, y);
}

/* Lets the pointer roam the whole 320x200 screen again. */
void UnpinMouse(void)
{
	SetMouseXRange(0, 639);
	SetMouseYRange(0, 199);
}

/* Runs on the game thread from plat_pump: records the position, then calls every attached handler
 * whose mask matches, last attached first, as the driver's callback did. */
static void DispatchMouseEvents(uint16_t events, uint16_t buttons, int16_t x, int16_t y)
{
	int16_t mask = 0;
	int16_t i;

	if (MouseDispatching)
		return;
	MouseDispatching = 1;
	if (events & PLAT_MOUSE_MOVED)
		mask |= MOUSE_MOVED;
	if (events & PLAT_MOUSE_LEFT_DOWN)
		mask |= 0x02;
	if (events & PLAT_MOUSE_LEFT_UP)
		mask |= 0x04;
	if (events & PLAT_MOUSE_RIGHT_DOWN)
		mask |= 0x08;
	if (events & PLAT_MOUSE_RIGHT_UP)
		mask |= 0x10;
	x = Clamp(x, MouseXMin, MouseXMax);
	y = Clamp(y, MouseYMin, MouseYMax);
	MouseX = x;
	MouseY = y;
	for (i = MouseHandlerCount - 1; i >= 0; i--)
		if (mask & MouseHandlerMasks[i])
			MouseHandlers[i](mask, buttons, x, y);
	MouseDispatching = 0;
}

MouseHandler::MouseHandler()
{
	slot = -1;
}

void MouseHandler::attach(MouseFunc fn, int16_t mask)
{
	if (MouseHandlerCount >= MOUSE_HANDLER_SLOTS)
		MOUSE_ERROR(331);
	slot = MouseHandlerCount;
	MouseHandlers[MouseHandlerCount] = fn;
	MouseHandlerMasks[MouseHandlerCount] = mask;
	MouseHandlerCount++;
	if (MouseCallbackOff == 0)
		plat_mouse_set_handler(DispatchMouseEvents);
}

/* The last one out stops the callback. */
void MouseHandler::detach()
{
	if (MouseCallbackOff == 0 && installed()) {
		MouseHandlers[slot] = 0;
		MouseHandlerMasks[slot] = 0;
		if (--MouseHandlerCount == 0)
			plat_mouse_set_handler(0);
	}
}

extern "C" void ResetMouseGlobals(void)
{
	MousePresent = 0;
	MouseButtonCount = 0;
	MouseCallbackOff = 0;
	MouseHandlerCount = 0;
	MouseX = 0;
	MouseY = 0;
	memset(MouseHandlers, 0, sizeof(MouseHandlers));
	memset(MouseHandlerMasks, 0, sizeof(MouseHandlerMasks));
	MouseStackOverrun = 0;
	MouseXMin = 0;
	MouseXMax = 639;
	MouseYMin = 0;
	MouseYMax = 199;
	MouseDispatching = 0;
}
