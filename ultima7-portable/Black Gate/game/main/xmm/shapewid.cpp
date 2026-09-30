/* Black Gate U7.EXE, resident segment 159 (file offsets 0x03f779 to 0x03f7f2, 121 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "u7manage.h"

/* The width of the shape's widest frame. */
int16_t GetMaxFrameWidth(int32_t shape, int16_t flags)
{
	Rect *rect;
	int16_t count;
	int16_t max;
	int16_t width;
	Rect bounds;
	int16_t i;

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
