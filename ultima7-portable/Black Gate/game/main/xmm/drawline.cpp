#include "u7port.h"
#include "arena.h"
#include "view.h"

/* Where a line crosses a clip edge: num * delta / span, all in 16 bits. */
static int16_t Interpolate(int16_t num, int16_t delta, int16_t span)
{
	return (int16_t)((int32_t)num * delta / span);
}

static void Swap(int16_t *a, int16_t *b)
{
	int16_t t = *a;

	*a = *b;
	*b = t;
}

/* Draw a line from x0, y0 to x1, y1 in one color, clipped to the view, stepping Bresenham style.
 * The row step comes from the first two rows, so rows must be evenly spaced.
 */
void DrawLine(View *view, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color)
{
	const Rect *clip = &view->clip;
	int32_t pitch = (int32_t)(LinearGet32(view->rowTable + 4) - LinearGet32(view->rowTable));
	int32_t rowStep, dst;
	int16_t topY, lowY, xSpan, ySpan, err;
	uint16_t count;
	bool upward;

	/* ends left to right */
	if (x1 < x0) {
		Swap(&x0, &x1);
		Swap(&y0, &y1);
	}
	upward = y1 < y0;
	topY = upward ? y1 : y0;
	lowY = upward ? y0 : y1;
	rowStep = upward ? -pitch : pitch;
	if (x1 < clip->x0 || x0 > clip->x1 || lowY < clip->y0 || topY > clip->y1)
		return;
	if (x1 > clip->x1) {
		y1 = Interpolate(clip->x1 - x0, y1 - y0, x1 - x0) + y0;
		x1 = clip->x1;
	}
	if (x0 < clip->x0) {
		y0 = Interpolate(clip->x0 - x0, y1 - y0, x1 - x0) + y0;
		x0 = clip->x0;
	}

	/* ends top to bottom while clipping y */
	if (upward) {
		Swap(&x0, &x1);
		Swap(&y0, &y1);
	}
	if (y1 < clip->y0 || y0 > clip->y1)
		return;
	if (y1 > clip->y1) {
		x1 = Interpolate(clip->y1 - y0, x1 - x0, y1 - y0) + x0;
		y1 = clip->y1;
	}
	if (y0 < clip->y0) {
		x0 = Interpolate(clip->y0 - y0, x1 - x0, y1 - y0) + x0;
		y0 = clip->y0;
	}
	ySpan = y1 - y0;
	if (upward) {
		Swap(&x0, &x1);
		Swap(&y0, &y1);
	}
	xSpan = x1 - x0;

	dst = (int32_t)LinearGet32(view->rowTable + (uint16_t)(y0 << 2)) + (uint16_t)x0;
	err = 0;
	if (x0 == x1) {
		count = ySpan + 1;
		do {
			LinearPut8(dst, color);
			dst += rowStep;
		} while (--count != 0);
	} else if (y0 == y1) {
		memset(LINEAR(dst), (uint8_t)color, (uint16_t)(xSpan + 1));
	} else if (ySpan > xSpan) {
		count = ySpan + 1;
		for (;;) {
			LinearPut8(dst, color);
			dst += rowStep;
			if (--count == 0)
				break;
			err += xSpan;
			if (err < ySpan)
				continue;
			dst++;
			err -= ySpan;
		}
	} else {
		count = xSpan + 1;
		for (;;) {
			LinearPut8(dst++, color);
			if (--count == 0)
				break;
			err += ySpan;
			if (err < xSpan)
				continue;
			dst += rowStep;
			err -= xSpan;
		}
	}
}
