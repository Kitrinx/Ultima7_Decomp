/* Black Gate INTRO.EXE, resident segment 80 (file offsets 0x01a274 to 0x01a2ca, 86 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"

/* A frame's bounds. The view header's Rect clears itself when made; this one is left alone. */
struct Rect {
	int x0, y0, x1, y1;
};

/* The width of the shape's widest frame. */
int far pascal GetMaxFrameWidth(void far *shape)
{
	int count;
	int max;
	int width;
	Rect bounds;
	int i;

	max = 0;
	count = GetShapeFrameCount(shape);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i);
		width = bounds.x1 - bounds.x0 + 1;
		if (width > max)
			max = width;
	}
	return max;
}
