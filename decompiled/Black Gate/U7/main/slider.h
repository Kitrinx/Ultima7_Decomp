#ifndef SLIDER_H
#define SLIDER_H

#include "gumps.h"

/* A button that repeats while held. */
struct RepeatButton : SizedSprite {
	RepeatButton(int shape) : SizedSprite(shape) {}
	unsigned char handle(MouseState *);
};

#endif
