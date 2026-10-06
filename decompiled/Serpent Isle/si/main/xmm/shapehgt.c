/* Serpent Isle SI.EXE, resident segment 157 (file offsets 0x03f2b2 to 0x03f3d4, 290 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "lowlevel.h"
#include "u7manage.h"
#include "framerct.h"

int far GetMaxFrameHeight(long shape, int flags)
{
	int count;
	int max;
	int height;
	int i;

	max = 0;
	FrameRect bounds;
	count = GetShapeFrameCount(shape, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i, flags);
		height = bounds.bottom() - bounds.top() + 1;
		if (height > max)
			max = height;
	}
	return max;
}
