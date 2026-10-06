/* Serpent Isle MAINMENU.EXE, resident segment 17 (file offsets 0x00f0ec to 0x00f245, 345 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 * Added method names and no-inline setting inferred from emitted bodies and calls.
 */

#include "view.h"
#include "lowlevel.h"

/* The union of the bounds of every frame of a shape. */
void GetShapeBounds(void far *data, Rect *bounds, int flags)
{
	int count;
	Rect frame;
	int i;

	bounds->set(0, 0, 0, 0);
	count = GetShapeFrameCount((long) data, flags);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&frame, 0, 0, (long) data, i, flags);
		if (frame.getY() < bounds->getY())
			bounds->setY(frame.getY());
		if (frame.getY1() > bounds->getY1())
			bounds->setY1(frame.getY1());
		if (frame.getX() < bounds->getX())
			bounds->setX(frame.getX());
		if (frame.getX1() > bounds->getX1())
			bounds->setX1(frame.getX1());
	}
}
