/* Serpent Isle SI.EXE, overlay segment 337 (file offsets 0x09d550 to 0x09ddb1, 2145 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "u7manage.h"
#include "objref.h"
#include "colbuf.h"
#include "gumps.h"
#include "slider.h"
#include "itable.h"
#include "u7event.h"
#include "bltshape.h"
#include "systimer.h"

struct View;

struct Rect : Point {
	int right, bottom;
	Rect(int x0, int y0, int x1, int y1) : Point(x0, y0) { right = x1; bottom = y1; }
	int contains(int px, int py) { return (px >= x) & (px <= right) & (py >= y) & (py <= bottom); }
};

/* A number picker: a thumb dragged along a bar, with step buttons either side. */
struct SliderGump : Control {
	ProportionalTextPrinter text;
	SizedSprite accept;
	RepeatButton down, up;
	int thumbShape, backgroundShape, endShape;
	int minimum, maximum, step;
	int left, right, x, y, value;
	SliderGump(int, int, int, int, int, int);
	void moveTo(int, int);
	void draw(View *);
	unsigned char handle(MouseState *);
	void drag();
	void stepDown();
	void stepUp();
	int valueToX(int);
	int xToValue(int);
};

extern View Viewport;
extern void far CopyFrameBuffer();

unsigned char RepeatButton::handle(MouseState *mouse)
{
	int px, py;
	Rect bounds(x - width, y - height, x, y);
	px = MouseState_getX(mouse);
	py = mouse->y;
	if (!bounds.contains(px, py))
		release();
	else if (mouse->action == MOUSE_CLICK) {
		press();
		return BUTTON_CLICKED;
	}
	return 0;
}

SliderGump::SliderGump(int low, int high, int increment, int initial, int px, int py)
	: accept(2), down(17), up(16)
{
	int width, height;
	MouseState unusedMouse;
	add(&accept);
	add(&down);
	add(&up);
	accept.show();
	down.show();
	up.show();
	minimum = low;
	maximum = high;
	step = increment;
	thumbShape = 1438;
	backgroundShape = 1437;
	endShape = 1436;
	if (px == -1 && py == -1) {
		px = MouseState_getX(GetLastMouseState()) - 10;
		py = GetLastMouseState()->y - 13;
	}
	gShapeManager.getFrameSize(&width, &height, backgroundShape, 0);
	if (width + px > 319)
		px = 319 - width;
	else if (px < 0)
		px = 0;
	if (height + py > 199)
		py = 199 - height;
	else if (py < 0)
		py = 0;
	moveTo(px, py);
	value = initial < minimum ? minimum : initial;
	text.setFont(2);
	text.spacing = 0;
	text.leading = 0;
}

void SliderGump::moveTo(int px, int py)
{
	x = px;
	y = py;
	left = x + 36;
	right = left + 55;
	accept.moveTo(x + 21, y + 18);
	down.moveTo(x + 31, y + 14);
	up.moveTo(x + 103, y + 14);
}

void SliderGump::draw(View *destination)
{
	if (visible) {
		View *previous = text.target;
		text.target = destination;
		ShapeManager_draw(&gShapeManager, destination, x, y, backgroundShape, 0, 0, 0);
		int thumbX = valueToX(value);
		int thumbY = y + 6;
		ShapeManager_draw(&gShapeManager, destination, thumbX, thumbY, thumbShape, 0, 0, 0);
		ShapeManager_draw(&gShapeManager, destination, x + 128, y + 16, endShape, 0, 0, 0);
		char number[4];
		itoa(value, number, 10);
		text.print(x + 108, y + 15, number);
		text.target = previous;
	}
}

void SliderGump::drag()
{
	paint(&Viewport);
	CopyFrameBuffer();
	do {
		if (UpdateAndGetMouseState()->moving()) {
			value = xToValue(MouseState_getX(GetLastMouseState()));
			value = (value / step) * step;
			if (value < minimum)
				value = minimum;
			else if (value > maximum)
				value = maximum;
			paint(&Viewport);
			CopyFrameBuffer();
		}
	} while (!GetLastMouseState()->released());
}

unsigned char SliderGump::handle(MouseState *mouse)
{
	unsigned char response;
	if (response = accept.handle(mouse)) {
		switch (response) {
		case BUTTON_CLICKED: return GUMP_CLOSE;
		}
	}
	Timer repeat;
	if (down.handle(mouse)) {
		do {
			UpdateAndCopyMouseState(mouse);
			stepDown();
			paint(&Viewport);
			CopyFrameBuffer();
			Timer_delay(&repeat, 5L);
		} while (!GetLastMouseState()->released());
		down.release();
		return GUMP_REDRAW;
	} else if (up.handle(mouse)) {
		do {
			UpdateAndCopyMouseState(mouse);
			stepUp();
			paint(&Viewport);
			CopyFrameBuffer();
			Timer_delay(&repeat, 5L);
		} while (!GetLastMouseState()->released());
		up.release();
		return GUMP_REDRAW;
	}
	if (gShapeManager.isCursorInBounds(thumbShape, 0,
		Point(valueToX(value), y + 6), Point(MouseState_getX(mouse), mouse->y)) &&
		mouse->action == MOUSE_CLICK) {
		drag();
	} else if (Rect(left, y + 6, right, y + 14).contains(MouseState_getX(mouse), mouse->y)) {
		value = xToValue(MouseState_getX(mouse));
		value = (value / step) * step;
		drag();
	}
	return 0;
}

void SliderGump::stepDown()
{
	if (value - step < minimum)
		value = minimum;
	else
		value -= step;
}

void SliderGump::stepUp()
{
	if (value + step > maximum)
		value = maximum;
	else
		value += step;
}

int SliderGump::valueToX(int amount)
{
	long scaled, origin;
	int range;
	long position;
	if (amount < minimum)
		return left;
	else if (amount > maximum)
		return right;
	else {
		range = maximum - minimum;
		scaled = 55L * (amount - minimum);
		origin = (long)left * range;
		position = scaled + origin;
		return position / range;
	}
}

int SliderGump::xToValue(int position)
{
	long scaled, origin;
	int range, result;
	if (position < left)
		return minimum;
	else if (position > right)
		return maximum;
	else {
		range = right - left;
		scaled = (long)(maximum - minimum) * (position - left);
		origin = (long)minimum * range;
		result = scaled + origin;
		return result / range;
	}
}
