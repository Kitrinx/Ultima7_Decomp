#ifndef SLIDER_H
#define SLIDER_H

#include "gumps.h"

/* A button that repeats while held. */
struct RepeatButton : SizedSprite {
	RepeatButton(int16_t shape) : SizedSprite(shape) {}
	uint8_t handle(MouseState *);
};

#endif
