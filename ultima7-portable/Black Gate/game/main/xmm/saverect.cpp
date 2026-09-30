#include "u7port.h"
#include "arena.h"
#include "view.h"
#include "saveunder.h"

void CopySaveBlock(View *view, int32_t buffer, int16_t left, int16_t top, int16_t right,
	int16_t bottom, bool restore)
{
	const Rect *clip = &view->clip;
	uint16_t boxWidth = (uint16_t)(right - left + 1);
	uint16_t cols = boxWidth;
	uint16_t rows = (uint16_t)(bottom - top + 1);
	int16_t clipLeft = left;
	int16_t clipTop = top;
	int32_t block, rowPtr;
	uint8_t *screen, *saved;

	if (left > clip->x1)
		return;
	if (clip->x0 >= left) {
		cols -= clip->x0 - left;
		clipLeft = clip->x0;
	}
	if (top > clip->y1)
		return;
	if (clip->y0 >= top) {
		rows -= clip->y0 - top;
		clipTop = clip->y0;
	}
	if (right < clip->x0)
		return;
	if (right > clip->x1)
		cols -= right - clip->x1;
	if (bottom < clip->y0)
		return;
	if (bottom > clip->y1)
		rows -= bottom - clip->y1;

	block = buffer + (uint16_t)((uint16_t)(clipTop - top) * boxWidth) + (uint16_t)(clipLeft - left);
	rowPtr = view->rowTable + (uint32_t)(uint16_t)clipTop * 4;
	do {
		screen = LINEAR(LinearGet32(rowPtr) + (uint16_t)clipLeft);
		saved = LINEAR(block);
		if (restore)
			memmove(screen, saved, cols);
		else
			memmove(saved, screen, cols);
		rowPtr += 4;
		block += boxWidth;
	} while (--rows != 0);
}

/* Save a rectangle of a view into a buffer, clipped to the view.
 * The buffer is always linear, so flags go unused.
 */
extern "C" void SaveRect(View *view, int32_t buffer, void *rect, int16_t flags)
{
	const Rect *r = (const Rect *)rect;

	CopySaveBlock(view, buffer, r->x0, r->y0, r->x1, r->y1, false);
}
