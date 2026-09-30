#include "u7port.h"
#include "arena.h"
#include "view.h"

/* Fill the box x0, y0 to x1, y1 with one color, clipped to the view.
 * The row step comes from the first two rows, so rows must be evenly spaced.
 */
void FillRectangle(View *view, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color)
{
	const Rect *clip = &view->clip;
	uint16_t width, rows;
	uint32_t pitch, rowSkip;
	int32_t dst;

	if (x0 > clip->x1)
		return;
	if (x0 < clip->x0)
		x0 = clip->x0;
	if (x1 < clip->x0)
		return;
	if (x1 > clip->x1)
		x1 = clip->x1;
	if (y0 > clip->y1)
		return;
	if (y0 < clip->y0)
		y0 = clip->y0;
	if (y1 < clip->y0)
		return;
	if (y1 > clip->y1)
		y1 = clip->y1;

	width = (uint16_t)(x1 - x0 + 1);
	pitch = LinearGet32(view->rowTable + 4) - LinearGet32(view->rowTable);
	/* only the low word of the pitch loses the width */
	rowSkip = (pitch & 0xffff0000) | (uint16_t)(pitch - width);
	dst = x0 + (int32_t)LinearGet32(view->rowTable + (uint16_t)(y0 << 2));
	rows = (uint16_t)(y1 - y0 + 1);
	do {
		memset(LINEAR(dst), (uint8_t)color, width);
		dst += width + rowSkip;
	} while (--rows != 0);
}
