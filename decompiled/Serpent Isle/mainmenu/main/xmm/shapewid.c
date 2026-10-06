/* Serpent Isle MAINMENU.EXE, resident segment 65 (file offsets 0x01924a to 0x0192c3, 121 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "view.h"
#include "u7manage.h"

int far GetMaxFrameWidth(long shape, int flags)
{
	int count;
	int max;
	int width;
	int i;

	max = 0;
	Rect bounds;
	count = GetShapeFrameCount(shape, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&bounds, 0, 0, shape, i, flags);
		width = bounds.x1 - bounds.x0 + 1;
		if (width > max)
			max = width;
	}
	return max;
}
