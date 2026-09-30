/* Black Gate U7.EXE, resident segment 38 (file offsets 0x01bf50 to 0x01c314, 964 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "plat.h"
#include "mouse.h"
#include "dbgfont.h"
#include "systimer.h"
#include "mevent.h"
#include "u7event.h"
#include "msclick.h"

int16_t MouseHand = 0;
MouseState LastMouseAction;
int16_t DoubleClickDelay = 0;
RomFontLoader RomFont;

static void OnMouseAction(int16_t mask, int16_t state, int16_t x, int16_t y);

MouseState::MouseState(int8_t action, int8_t button, int16_t x, int16_t y)
{
	type = 0;
	this->action = action;
	this->button = button;
	this->x = x;
	this->y = y;
	time = TickCount;
}

MouseQueue::MouseQueue(char *buf, int16_t count) : EventQueue(buf, sizeof(MouseState), count)
{
}

MouseQueue::MouseQueue(char *buf, int16_t size, int16_t count) : EventQueue(buf, size, count)
{
}

void MouseQueue::install()
{
	attach(OnMouseAction, MOUSE_BUTTONS);
}

/* Called by the mouse driver; both buttons at the top left corner show the debug line. */
static void OnMouseAction(int16_t mask, int16_t state, int16_t x, int16_t y)
{
	MouseState ev;

	if ((state & 3) == 3 && x == 0 && y == 0) {
		DrawDebugArea();
		CallDebugHook();
	}
	ev.x = x;
	ev.y = y;
	ev.time = TickCount;
	ev.state = state;
	if (mask & MOUSE_LEFT)
		ev.button = 1;
	else if (mask & MOUSE_RIGHT)
		ev.button = 2;
	else
		ev.button = 0;
	ev.action = 0;
	if (mask & MOUSE_RELEASED)
		ev.type = MOUSE_EVENT_RELEASED;
	else if (mask & MOUSE_PRESSED)
		ev.type = MOUSE_EVENT_PRESSED;
	else
		ev.type = 0;
	EnqueueMouseEvent(&ev);
}

/* Swaps the buttons for a left-handed player. */
void ApplyMouseHand(MouseState *e)
{
	int8_t button = e->button;
	int16_t state = e->state;

	if (MouseHand != 0) {
		if (button == 1)
			button = 2;
		else if (button == 2)
			button = 1;
		if ((state & 3) == 0)
			state ^= 3;
	}
	e->button = button;
	e->state = state;
}

/* Turn the queued presses and releases of one button into a click, a double click or a release. */
uint8_t ClassifyMouseClick(MouseState *e, int16_t delay)
{
	int8_t button;
	uint32_t now;
	MouseState *second, *third;
	MouseState *head;

	now = TickCount;
	head = (MouseState *)MouseQueueHead;
	second = (MouseState *)NextMouseSlot(MouseQueueHead);
	third = (MouseState *)NextMouseSlot((char *)second);
	if (IsQueuedMouseEvent((char *)head))
		button = head->button;
	else
		head = 0;
	if (IsQueuedMouseEvent((char *)second)) {
		if (second->button != button)
			second = 0;
	} else
		second = 0;
	if (IsQueuedMouseEvent((char *)third)) {
		if (third->button != button)
			third = 0;
	} else
		third = 0;
	if (head == 0)
		return 0;
	if (head->type == MOUSE_EVENT_RELEASED) {
		DequeueMouseEvent(e);
		ApplyMouseHand(e);
		e->action = MOUSE_RELEASE;
		return 1;
	}
	if (head->type == MOUSE_EVENT_PRESSED) {
		if (second != 0) {
			if (second->time - head->time <= delay) {
				if (third != 0) {
					DequeueMouseEvent(e);
					DequeueMouseEvent(e);
					DequeueMouseEvent(e);
					ApplyMouseHand(e);
					e->action = MOUSE_DOUBLE_CLICK;
					return 1;
				} else if (now - second->time <= delay)
					return 0;
				else {
					DequeueMouseEvent(e);
					ApplyMouseHand(e);
					e->action = MOUSE_CLICK;
					return 1;
				}
			} else {
				DequeueMouseEvent(e);
				ApplyMouseHand(e);
				e->action = MOUSE_CLICK;
				return 1;
			}
		} else if (now - head->time <= delay)
			return 0;
		DequeueMouseEvent(e);
		ApplyMouseHand(e);
		e->action = MOUSE_CLICK;
		return 1;
	}
	return 0;
}

/* Handles the host's events first, so the queue and the pointer are current. */
MouseState *GetMouseAction(void)
{
	plat_pump();
	LastMouseAction.action = 0;
	if (!ClassifyMouseClick(&LastMouseAction, DoubleClickDelay))
		CheckPointerMoved();
	return &LastMouseAction;
}

MouseState *CopyMouseAction(MouseState *e)
{
	GetMouseAction();
	*e = LastMouseAction;
	return e;
}

/* Reports a pointer move as an event of its own. */
uint8_t CheckPointerMoved(void)
{
	if (GetMouseX() != LastMouseAction.x || GetMouseY() != LastMouseAction.y) {
		LastMouseAction.action = MOUSE_MOVE;
		LastMouseAction.x = GetMouseX();
		LastMouseAction.y = GetMouseY();
		return 1;
	}
	LastMouseAction.action = 0;
	return 0;
}
