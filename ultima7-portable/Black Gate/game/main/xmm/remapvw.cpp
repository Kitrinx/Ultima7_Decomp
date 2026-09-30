#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"

/* Pass every pixel of the view's clip box through a 256-byte color table, in place. */
void RemapView(View view, int32_t colors)
{
	uint32_t left = (uint16_t)view.clip.x0;
	uint16_t cols = (uint16_t)(view.clip.x1 - view.clip.x0 + 1);
	uint16_t rows = (uint16_t)(view.clip.y1 - view.clip.y0 + 1);
	int32_t rowPtr = view.rowTable + (uint32_t)(uint16_t)view.clip.y0 * 4;
	const uint8_t *table = LINEAR(colors);
	uint8_t *p;
	uint16_t n;

	if (rows == 0)
		return;
	do {
		p = LINEAR(LinearGet32(rowPtr) + left);
		n = cols;
		do {
			*p = table[*p];
			p++;
		} while (--n != 0);
		rowPtr += 4;
	} while (--rows != 0);
}
