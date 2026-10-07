/* Serpent Isle MAINMENU.EXE, resident segment 17 (file offsets 0x00f0ec to 0x00f245, 345 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 * Added method names and no-inline setting inferred from emitted bodies and calls.
 */

#include "u7port.h"
#include "view.h"
#include "lowlevel.h"
#include "dosio.h"

namespace MainMenu {

/* The union of the bounds of every frame of a shape. */
void GetShapeBounds(void *data, Rect *bounds, int16_t flags)
{
	int16_t count;
	Rect frame;
	int16_t i;

	bounds->set(0, 0, 0, 0);
	count = GetShapeFrameCount(PointerToLinear(data), flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&frame, 0, 0, PointerToLinear(data), i, flags);
		if (frame.y < bounds->y)
			bounds->y = frame.y;
		if (frame.y1 > bounds->y1)
			bounds->y1 = frame.y1;
		if (frame.x < bounds->x)
			bounds->x = frame.x;
		if (frame.x1 > bounds->x1)
			bounds->x1 = frame.x1;
	}
}

}
