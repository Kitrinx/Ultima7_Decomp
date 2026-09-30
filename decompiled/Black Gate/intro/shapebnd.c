/* Black Gate INTRO.EXE, resident segment 10 (file offsets 0x00b125 to 0x00b1ca, 165 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "lowlevel.h"

/* The union of the bounds of every frame of a shape. */
void GetShapeBounds(void far *data, Rect *bounds)
{
	int count;
	Rect frame;
	int i;

	bounds->x0 = 0;
	bounds->y0 = 0;
	bounds->x1 = 0;
	bounds->y1 = 0;
	count = GetShapeFrameCount(data);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&frame, 0, 0, data, i);
		if (frame.y0 < bounds->y0)
			bounds->y0 = frame.y0;
		if (frame.y1 > bounds->y1)
			bounds->y1 = frame.y1;
		if (frame.x0 < bounds->x0)
			bounds->x0 = frame.x0;
		if (frame.x1 > bounds->x1)
			bounds->x1 = frame.x1;
	}
}
