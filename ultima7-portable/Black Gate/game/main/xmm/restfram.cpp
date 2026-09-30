#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"
#include "saveunder.h"

/* Copy the block SaveUnderFrame saved back onto the view.
 * Buffer and shape are always linear, so flags go unused.
 */
void RestoreUnderFrame(View *view, int32_t buffer, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	int16_t flags)
{
	uint32_t entry = (uint16_t)((frameNum + 1) << 2);
	int32_t frame;

	if (LinearGet32(shape + 4) <= entry)
		return;
	frame = shape + (int32_t)LinearGet32(shape + entry);
	CopySaveBlock(view, buffer, x - (int16_t)LinearGet16(frame + 2), y - (int16_t)LinearGet16(frame + 4),
		x + (int16_t)LinearGet16(frame), y + (int16_t)LinearGet16(frame + 6), true);
}
