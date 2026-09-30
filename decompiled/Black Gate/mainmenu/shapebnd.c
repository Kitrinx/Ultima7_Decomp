/* Black Gate MAINMENU.EXE, resident segment 17 (file offsets 0x00e867 to 0x00e918, 177 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "lowlevel.h"

/* The union of the bounds of every frame of a shape. */
void GetShapeBounds(void far *data, Rect *bounds, int flags)
{
	int count;
	Rect frame;
	int i;

	bounds->x0 = 0;
	bounds->y0 = 0;
	bounds->x1 = 0;
	bounds->y1 = 0;
	count = GetShapeFrameCount((long) data, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&frame, 0, 0, (long) data, i, flags);
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
