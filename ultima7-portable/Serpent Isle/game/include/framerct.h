#ifndef FRAMERCT_H
#define FRAMERCT_H

#include "geometry.h"

/* A frame's bounds. Laid out as Rect, but its own class: these two modules keep their own copies
 * of top() and bottom() beside drawbuf.c's. */
struct FrameRect : Point {
	int16_t x1, y1;
	FrameRect() : Point(0, 0) { x1 = 0; y1 = 0; }
	int16_t bottom() { return y1; }
	int16_t top() { return y; }
	int16_t right() { return x1; }
	int16_t left() { return x; }
};

#endif
