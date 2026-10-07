#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

/* A shape is its size, then a table of frame offsets that ends where the first frame starts.
 * Shape addresses are always linear, so flags go unused.
 */
int16_t GetShapeFrameCount(int32_t shape, int16_t flags)
{
	return (int16_t)((LinearGet16(shape + 4) >> 2) - 1);
}
