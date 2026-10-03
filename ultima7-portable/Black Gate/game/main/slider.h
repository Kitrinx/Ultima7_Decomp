#ifndef SLIDER_H
#define SLIDER_H

#include "gumps.h"
#include "itable.h"

/* A button that repeats while held. */
struct RepeatButton : SizedSprite {
	RepeatButton(int16_t shape) : SizedSprite(shape) {}
	uint8_t handle(MouseState *);
};

/* A number picker: a thumb dragged along a bar, with step buttons either side. */
struct SliderGump : Control {
	ProportionalTextPrinter text;
	SizedSprite accept;
	RepeatButton down, up;
	int16_t thumbShape, backgroundShape, endShape;
	int16_t minimum, maximum, step;
	int16_t left, right, x, y, value;
	SliderGump(int16_t, int16_t, int16_t, int16_t, int16_t, int16_t);
	void moveTo(int16_t, int16_t);
	void draw(View *);
	uint8_t handle(MouseState *);
	void drag();
	void stepDown();
	void stepUp();
	int16_t valueToX(int16_t);
	int16_t xToValue(int16_t);
};

#endif
