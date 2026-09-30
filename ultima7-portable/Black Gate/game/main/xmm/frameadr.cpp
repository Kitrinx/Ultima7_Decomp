#include "u7port.h"
#include "lowlevel.h"

/* Return one frame of a shape, or 0 if its table entry is empty.
 * The frame number may run one past the table; that reads the first frame's header.
 */
void *GetFrameAddress(char *shape, int16_t frameNum, int16_t flags)
{
	uint8_t *s = (uint8_t *)shape;
	uint16_t entry = (uint16_t)((frameNum + 1) << 2);
	uint32_t offset;

	if ((int16_t)entry > (int16_t)(s[4] | s[5] << 8))
		return 0;
	offset = s[entry] | s[entry + 1] << 8 | (uint32_t)s[entry + 2] << 16 | (uint32_t)s[entry + 3] << 24;
	if (offset == 0)
		return 0;
	return s + offset;
}
