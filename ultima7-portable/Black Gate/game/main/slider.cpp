/* Black Gate U7.EXE, overlay segment 347 (file offsets 0x0a6e40 to 0x0a76a1, 2145 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
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
	int16_t right, bottom;
	Rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1) : Point(x0, y0) { right = x1; bottom = y1; }
	int16_t contains(int16_t px, int16_t py) { return (px >= x) & (px <= right) & (py >= y) & (py <= bottom); }
};

extern View Viewport;
extern void CopyFrameBuffer();

uint8_t RepeatButton::handle(MouseState *mouse)
{
	int16_t px, py;
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

SliderGump::SliderGump(int16_t low, int16_t high, int16_t increment, int16_t initial, int16_t px, int16_t py)
	: accept(2), down(17), up(16)
{
	int16_t width, height;
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
	thumbShape = 1374;
	backgroundShape = 1373;
	endShape = 1372;
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

void SliderGump::moveTo(int16_t px, int16_t py)
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
		int16_t thumbX = valueToX(value);
		int16_t thumbY = y + 6;
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
		plat_yield();
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

uint8_t SliderGump::handle(MouseState *mouse)
{
	uint8_t response;
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
			Timer_delay(&repeat, INT32_C(5));
		} while (!GetLastMouseState()->released());
		down.release();
		return GUMP_REDRAW;
	} else if (up.handle(mouse)) {
		do {
			UpdateAndCopyMouseState(mouse);
			stepUp();
			paint(&Viewport);
			CopyFrameBuffer();
			Timer_delay(&repeat, INT32_C(5));
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

int16_t SliderGump::valueToX(int16_t amount)
{
	int32_t scaled, origin;
	int16_t range;
	int32_t position;
	if (amount < minimum)
		return left;
	else if (amount > maximum)
		return right;
	else {
		range = maximum - minimum;
		scaled = INT32_C(55) * (amount - minimum);
		origin = (int32_t)left * range;
		position = scaled + origin;
		return position / range;
	}
}

int16_t SliderGump::xToValue(int16_t position)
{
	int32_t scaled, origin;
	int16_t range, result;
	if (position < left)
		return minimum;
	else if (position > right)
		return maximum;
	else {
		range = right - left;
		scaled = (int32_t)(maximum - minimum) * (position - left);
		origin = (int32_t)minimum * range;
		result = scaled + origin;
		return result / range;
	}
}
