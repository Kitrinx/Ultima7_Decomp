/* Black Gate U7.EXE, resident segment 34 (file offsets 0x01a464 to 0x01a73b, 727 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <new>
#include "mouse.h"
#include "mevent.h"
#include "u7event.h"

int16_t MouseEventSize = 0;
char *MouseQueueStart = 0;
char *MouseQueueEnd = 0;
char *MouseQueueHead = 0;
char *MouseQueueTail = 0;

MouseEvent LastMouseEvent;

static void AdvanceMouseHead(void);
static void OnMouseEvent(int16_t, int16_t, int16_t, int16_t);

EventQueue::EventQueue(char *buf, int16_t count)
{
	MouseEventSize = sizeof(MouseEvent);
	MouseQueueStart = buf;
	MouseQueueEnd = MouseQueueStart + MouseEventSize * count;
	MouseQueueHead = MouseQueueTail = buf;
}

EventQueue::EventQueue(char *buf, int16_t size, int16_t count)
{
	MouseEventSize = size;
	MouseQueueStart = buf;
	MouseQueueEnd = MouseQueueStart + MouseEventSize * count;
	MouseQueueHead = MouseQueueTail = buf;
}

void EventQueue::install()
{
	attach(OnMouseEvent, MOUSE_BUTTONS);
}

static void AdvanceMouseTail(void)
{
	MouseQueueTail += MouseEventSize;
	if (MouseQueueTail >= MouseQueueEnd)
		MouseQueueTail = MouseQueueStart;
	if (MouseQueueTail == MouseQueueHead)
		AdvanceMouseHead();
}

int8_t IsQueuedMouseEvent(char *p)
{
	if (MouseQueueHead == MouseQueueTail)
		return 0;
	if (MouseQueueHead < MouseQueueTail)
		return p >= MouseQueueHead && p < MouseQueueTail;
	else
		return p >= MouseQueueHead || p < MouseQueueTail;
}

char *NextMouseSlot(char *p)
{
	char *next;

	next = p + MouseEventSize;
	if (next >= MouseQueueEnd)
		return MouseQueueStart;
	return next;
}

static void AdvanceMouseHead(void)
{
	MouseQueueHead += MouseEventSize;
	if (MouseQueueHead >= MouseQueueEnd)
		MouseQueueHead = MouseQueueStart;
}

void EnqueueMouseEvent(void *e)
{
	memcpy(MouseQueueTail, e, MouseEventSize);
	AdvanceMouseTail();
}

uint8_t DequeueMouseEvent(void *e)
{
	if (MouseQueueHead != MouseQueueTail) {
		memcpy(e, MouseQueueHead, MouseEventSize);
		AdvanceMouseHead();
		return 1;
	}
	return 0;
}

static void OnMouseEvent(int16_t mask, int16_t state, int16_t x, int16_t y)
{
	MouseEvent ev;

	ev.x = x;
	ev.y = y;
	ev.state = state;
	if (mask & MOUSE_LEFT)
		ev.button = 1;
	else if (mask & MOUSE_RIGHT)
		ev.button = 2;
	else
		ev.button = 0;
	if (mask & MOUSE_PRESSED)
		ev.type = MOUSE_EVENT_PRESSED;
	else if (mask & MOUSE_RELEASED)
		ev.type = MOUSE_EVENT_RELEASED;
	else
		ev.type = 0;
	EnqueueMouseEvent(&ev);
}

void SkipToMouseRelease(void)
{
	while (MouseQueueHead != MouseQueueTail && ((MouseEvent *) MouseQueueHead)->type != MOUSE_EVENT_RELEASED) {
		MouseQueueHead += MouseEventSize;
		if (MouseQueueHead >= MouseQueueEnd)
			MouseQueueHead = MouseQueueStart;
	}
}

MouseEvent *GetMouseEvent(void)
{
	LastMouseEvent.type = 0;
	if (!DequeueMouseEvent(&LastMouseEvent))
		CheckMouseMoved();
	return &LastMouseEvent;
}

MouseEvent *CopyMouseEvent(MouseEvent *e)
{
	GetMouseEvent();
	*e = LastMouseEvent;
	return e;
}

uint8_t CheckMouseMoved(void)
{
	if (GetMouseX() != LastMouseEvent.x || GetMouseY() != LastMouseEvent.y) {
		LastMouseEvent.type = MOUSE_EVENT_MOVED;
		LastMouseEvent.x = GetMouseX();
		LastMouseEvent.y = GetMouseY();
		return 1;
	}
	LastMouseEvent.type = 0;
	return 0;
}

void FlushMouseToRelease(void)
{
	SkipToMouseRelease();
}

extern "C" void ResetMeventGlobals(void)
{
	MouseEventSize = 0;
	MouseQueueStart = 0;
	MouseQueueEnd = 0;
	MouseQueueHead = 0;
	MouseQueueTail = 0;
	memset((void *)&LastMouseEvent, 0, sizeof(LastMouseEvent));
}

extern "C" void ConstructMeventGlobals(void)
{
	new (&LastMouseEvent) MouseEvent();
}
