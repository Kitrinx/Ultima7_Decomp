/* Black Gate INTRO.EXE, resident segment 81 (file offsets 0x01a2ca to 0x01a320, 86 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"

/* A frame's bounds. The view header's Rect clears itself when made; this one is left alone. */
struct Rect {
	int x0, y0, x1, y1;
};

/* The height of the shape's tallest frame. */
int far pascal GetMaxFrameHeight(void far *shape)
{
	int count;
	int max;
	int height;
	Rect bounds;
	int i;

	max = 0;
	count = GetShapeFrameCount(shape);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i);
		height = bounds.y1 - bounds.y0 + 1;
		if (height > max)
			max = height;
	}
	return max;
}
