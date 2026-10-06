/* Serpent Isle SI.EXE, resident segment 158 (file offsets 0x03f3d4 to 0x03f46c, 152 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "lowlevel.h"
#include "u7manage.h"
#include "framerct.h"

int far GetMaxFrameWidth(long shape, int flags)
{
	int count;
	int max;
	int width;
	int i;

	max = 0;
	FrameRect bounds;
	count = GetShapeFrameCount(shape, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i, flags);
		width = bounds.right() - bounds.left() + 1;
		if (width > max)
			max = width;
	}
	return max;
}
