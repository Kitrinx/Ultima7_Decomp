/* Black Gate U7.EXE, resident segment 161 (file offsets 0x03f831 to 0x03f885, 84 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"

char far TestShapeHit(char far *shape, int frame, void *pos, int posSegment, void *point, int pointSegment, int flags)
{
	int mode;
	void far *frameData;

	mode = flags;
	if (flags & 1)
		mode |= 0x100;
	if ((frameData = GetFrameAddress(shape, frame, mode)) == 0)
		return 0;
	return IsPointInFrame(frameData, pos, point, flags);
}
