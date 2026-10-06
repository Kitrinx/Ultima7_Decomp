/* Black Gate U7.EXE, resident segment 159 (file offsets 0x03f779 to 0x03f7f2, 121 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "view.h"
#include "u7manage.h"

/* The width of the shape's widest frame. */
int far GetMaxFrameWidth(long shape, int flags)
{
	Rect *rect;
	int count;
	int max;
	int width;
	Rect bounds;
	int i;

	max = 0;
	rect = &bounds;
	rect->x0 = 0;
	rect->y0 = 0;
	rect->x1 = 0;
	rect->y1 = 0;
	count = GetShapeFrameCount(shape, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i, flags);
		width = bounds.x1 - bounds.x0 + 1;
		if (width > max)
			max = width;
	}
	return max;
}
