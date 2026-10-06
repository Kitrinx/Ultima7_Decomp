/* Serpent Isle MAINMENU.EXE, resident segment 64 (file offsets 0x0191d1 to 0x01924a, 121 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "view.h"
#include "u7manage.h"

int far GetMaxFrameHeight(long shape, int flags)
{
	int count;
	int max;
	int height;
	int i;

	max = 0;
	Rect bounds;
	count = GetShapeFrameCount(shape, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i, flags);
		height = bounds.y1 - bounds.y0 + 1;
		if (height > max)
			max = height;
	}
	return max;
}
