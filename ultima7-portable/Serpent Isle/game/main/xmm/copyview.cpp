#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"

/* Copy one view onto another, row by row, over the width and height both share.
 * Both row tables start at the source view's top line.
 */
void CopyView(View *from, View *to)
{
	uint32_t srcLeft = (uint16_t)from->clip.x;
	uint32_t dstLeft = (uint16_t)to->clip.x;
	int16_t cols = from->clip.x1 - from->clip.x + 1;
	int16_t rows = from->clip.y1 - from->clip.y + 1;
	int16_t n;
	uint16_t count;
	int32_t src, dst;

	if (cols == 0)
		return;
	n = to->clip.x1 - to->clip.x + 1;
	if (n < cols)
		cols = n;
	n = to->clip.y1 - to->clip.y + 1;
	if (n < rows)
		rows = n;
	if (rows == 0)
		return;
	src = from->rowTable.get() + (uint32_t)(uint16_t)from->clip.y * 4;
	dst = to->rowTable.get() + (uint32_t)(uint16_t)from->clip.y * 4;
	count = (uint16_t)rows;
	do {
		memmove(LINEAR(LinearGet32(dst) + dstLeft), LINEAR(LinearGet32(src) + srcLeft), (uint16_t)cols);
		src += 4;
		dst += 4;
	} while (--count != 0);
}
