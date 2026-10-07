/* Serpent Isle SI.EXE, resident segment 160 (file offsets 0x03f4ab to 0x03f4ff, 84 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

int8_t TestShapeHit(int32_t shape, int16_t frame, void *pos, void *point, int16_t flags)
{
	int16_t mode;
	void *frameData;

	mode = flags;
	if (flags & 1)
		mode |= 0x100;
	if ((frameData = GetFrameAddress((char *)LINEAR(shape), frame, mode)) == 0)
		return 0;
	return IsPointInFrame(frameData, pos, point, flags);
}
