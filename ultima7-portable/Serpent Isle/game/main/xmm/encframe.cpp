#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"

/* Where EncodeFrame writes; with no destination it only counts. */
struct FrameWriter {
	int32_t at;
	bool writing;
	uint32_t total;
};

static void PutWord(FrameWriter *w, int16_t value)
{
	if (w->writing)
		LinearPut16(w->at, (uint16_t)value);
	w->at += 2;
	w->total += 2;
}

/* Run-length pack a span: count*2+1 then a byte repeated, or count*2 then that many bytes.
 * Returns the packed length.
 */
static int16_t PackSpan(const uint8_t *raw, int16_t rawLen, uint8_t *packed)
{
	int16_t packLen = 0;
	int16_t pos = 0;
	int16_t repeats, countAt;
	uint8_t pixel;

	while (pos < rawLen) {
		pixel = raw[pos++];
		repeats = 1;
		if (pos == rawLen) {
			packed[packLen++] = (uint8_t)(repeats * 2);
			packed[packLen++] = pixel;
			break;
		}
		if (raw[pos] == pixel) {
			do {
				repeats++;
				pos++;
			} while (pos != rawLen && raw[pos] == pixel && repeats != 127);
			packed[packLen++] = (uint8_t)(repeats * 2 | 1);
			packed[packLen++] = pixel;
			continue;
		}
		countAt = packLen++;
		for (;;) {
			packed[packLen++] = pixel;
			if (pos == rawLen || repeats == 127)
				break;
			if (raw[pos] == pixel) {
				/* the last pixel starts a repeat; give it back */
				pos--;
				repeats--;
				packLen--;
				break;
			}
			pixel = raw[pos++];
			repeats++;
		}
		packed[countAt] = (uint8_t)(repeats * 2);
	}
	return packLen;
}

/* Encode a box of a view as a shape frame: extents from the hot spot, then one span per run of
 * pixels other than keyColor, each packed when that gains at least minGain percent (below 100).
 * Corners and hot spot are x in the low word, y in the high.
 * Writes to dest when it is non-zero; returns the byte total, or 0 past 64K.
 */
uint16_t EncodeFrame(View *view, int32_t topLeft, int32_t bottomRight, int32_t hotSpot, int32_t dest,
	int16_t keyColor, int16_t minGain)
{
	int16_t left = (int16_t)topLeft, top = (int16_t)(topLeft >> 16);
	int16_t right = (int16_t)bottomRight, bottom = (int16_t)(bottomRight >> 16);
	int16_t hotX = (int16_t)hotSpot, hotY = (int16_t)(hotSpot >> 16);
	uint8_t key = (uint8_t)keyColor;
	uint8_t raw[SCREEN_WIDTH];
	uint8_t packed[2 * SCREEN_WIDTH];
	FrameWriter w = { dest, dest != 0, 0 };
	int16_t row, col, x, rawLen, packLen, length;
	int32_t src, spanHead;
	const uint8_t *data;
	uint16_t count;
	uint8_t pixel;

	PutWord(&w, right - hotX);
	PutWord(&w, hotX - left);
	PutWord(&w, hotY - top);
	PutWord(&w, bottom - hotY);
	for (row = top; row <= bottom; row++) {
		col = left;
		src = (int32_t)LinearGet32(view->rowTable.get() + (uint32_t)(uint16_t)row * 4) + col;
		while (col <= right) {
			if (LinearGet8(src) == key) {
				src++;
				col++;
				continue;
			}
			spanHead = w.at;
			PutWord(&w, 0);
			PutWord(&w, col - hotX);
			PutWord(&w, row - hotY);
			rawLen = 0;
			for (x = col; (pixel = LinearGet8(src)) != key && x <= right; x++, src++)
				raw[rawLen++] = pixel;

			packLen = rawLen;
			if (minGain < 100)
				packLen = PackSpan(raw, rawLen, packed);
			length = rawLen * 2;
			data = raw;
			count = (uint16_t)rawLen;
			if (rawLen - packLen >= 0 && (uint32_t)(uint16_t)(rawLen - packLen) * 100 <= 0xffff
				&& (int16_t)((rawLen - packLen) * 100) >= minGain) {
				length |= 1;
				data = packed;
				count = (uint16_t)packLen;
			}
			if (w.writing) {
				LinearPut16(spanHead, (uint16_t)length);
				memcpy(LINEAR(w.at), data, count);
				/* as the original did: disturbs the next four bytes, which later words mostly cover */
				LinearPut32(w.at + count, LinearGet32(w.at + count) - (uint16_t)spanHead);
			}
			w.at += count;
			w.total += count;
			col += rawLen;
		}
	}
	PutWord(&w, 0);
	if (w.total >> 16)
		return 0;
	return (uint16_t)w.total;
}
