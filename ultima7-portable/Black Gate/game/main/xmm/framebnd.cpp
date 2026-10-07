#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"

/* A word of a frame header. A tile shape has no frame table, so its pixels read as an offset can
 * point far past memory, where DOS read all ones. */
static int16_t FrameWord(int32_t address)
{
	if ((uint32_t)address > LinearSize - 2)
		return -1;
	return (int16_t)LinearGet16(address);
}

/* Fill bounds with the screen box of a frame drawn at x, y.
 * A frame starts with its extents from the hot spot: right, left, above, below.
 * Returns -1, or 0 when the frame number is past the table.
 */
int16_t GetFrameBounds(void *bounds, int16_t x, int16_t y, int32_t shape, int16_t frameNum, int16_t flags)
{
	Rect *r = (Rect *)bounds;
	uint16_t entry = (uint16_t)(frameNum << 2);
	int32_t frame;

	if ((int16_t)entry >= (int16_t)LinearGet16(shape + 4))
		return 0;
	frame = shape + (int32_t)LinearGet32(shape + (uint16_t)(entry + 4));
	r->x1 = x + FrameWord(frame);
	r->x0 = x - FrameWord(frame + 2);
	r->y0 = y - FrameWord(frame + 4);
	r->y1 = y + FrameWord(frame + 6);
	return -1;
}
