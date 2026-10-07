#include "u7port.h"
#include "arena.h"
#include "view.h"
#include "shapedraw.h"

/* A shape starts with its size and a table of 32-bit frame offsets; the first offset
 * also marks where the table ends. A frame starts with four 16-bit extents from its
 * hot spot, then its spans, each:
 *
 *   header    length * 2, plus 1 if run-encoded; a length of 0 ends the frame
 *   offsets   along the span, then across it, from the hot spot
 *   pixels    raw bytes, or runs of count * 2 plus 1 for a fill of the byte after it
 */
#define SHAPE_FIRST_FRAME       4
#define FRAME_HEADER_SIZE       8

static uint8_t SpanPixels[0x8000 + 0x80];

/* Unpacks one span into SpanPixels and returns the address just past it. */
static int32_t UnpackSpan(int32_t source, int32_t length, bool runEncoded)
{
	uint8_t *out = SpanPixels;

	if (!runEncoded) {
		memcpy(out, LINEAR(source), (size_t)length);
		return source + length;
	}
	while (length > 0) {
		uint8_t run = LinearGet8(source++);
		int32_t count = run >> 1;

		if (run & 1)
			memset(out, LinearGet8(source++), (size_t)count);
		else {
			memcpy(out, LINEAR(source), (size_t)count);
			source += count;
		}
		out += count;
		length -= count;
	}
	return source;
}

static void PutPixels(uint8_t *screen, int32_t step, const uint8_t *pixels, int32_t count,
	enum ShapePixelMode mode, const uint8_t *table)
{
	for (; count > 0; count--, screen += step, pixels++) {
		uint8_t mix;

		switch (mode) {
		case SHAPE_COPY:
			*screen = *pixels;
			break;
		case SHAPE_TRANSLATE:
			*screen = table[*screen];
			break;
		case SHAPE_BLEND:
			/* 0xff keeps the pixel solid; otherwise it picks the 256-byte
			 * table, counted from the table's start, that recolors the screen. */
			mix = table[*pixels];
			*screen = mix == 0xff ? *pixels : table[mix * 256 + *screen];
			break;
		}
	}
}

void DrawShapeFrame(struct View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	bool flipped, enum ShapePixelMode mode, int32_t table)
{
	uint16_t entry = (uint16_t)((frameNum + 1) * 4);
	const uint8_t *tableBytes = LINEAR(table);
	int32_t source;
	Rect *clip = &view->clip;

	if (LinearGet16(shape + SHAPE_FIRST_FRAME) < entry)
		return;
	source = shape + (int32_t)LinearGet32(shape + entry) + FRAME_HEADER_SIZE;
	for (;;) {
		uint16_t header = LinearGet16(source);
		int32_t length = header >> 1;
		int16_t along, across;
		int16_t alongMin, alongMax, acrossMin, acrossMax;
		int32_t first, last, row, column;

		if (length == 0)
			return;
		if (flipped) {
			along = (int16_t)(y + (int16_t)LinearGet16(source + 2));
			across = (int16_t)(x + (int16_t)LinearGet16(source + 4));
			alongMin = clip->y, alongMax = clip->y1;
			acrossMin = clip->x, acrossMax = clip->x1;
		} else {
			along = (int16_t)(x + (int16_t)LinearGet16(source + 2));
			across = (int16_t)(y + (int16_t)LinearGet16(source + 4));
			alongMin = clip->x, alongMax = clip->x1;
			acrossMin = clip->y, acrossMax = clip->y1;
		}
		source = UnpackSpan(source + 6, length, header & 1);

		if (across < acrossMin || across > acrossMax || along > alongMax
			|| (int16_t)(along + length - 1) < alongMin)
			continue;
		/* The visible pixels of the span, counted from its start. */
		first = alongMin > along ? alongMin - along : 0;
		last = alongMax - along < length - 1 ? alongMax - along : length - 1;
		if (first > last)
			continue;

		/* Flipped spans step a whole screen row per pixel, whatever the view's row table. */
		row = flipped ? along + first : across;
		column = flipped ? across : along + first;
		PutPixels(LINEAR(LinearGet32(view->rowTable.get() + row * 4) + column),
			flipped ? SCREEN_WIDTH : 1, SpanPixels + first, last - first + 1, mode, tableBytes);
	}
}

extern "C" void ResetShapedrawGlobals(void)
{
	memset(SpanPixels, 0, sizeof(SpanPixels));
}
