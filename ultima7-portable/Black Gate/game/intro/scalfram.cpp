/* Black Gate INTRO.EXE, the scaled frame drawing family: scalfram.asm (placement), shrink.asm,
 * shrturn.asm and enlarge.asm. The original picks one of 32 draw routines by size, angle and flip;
 * the intro only ever draws unflipped frames, shrunk or enlarged upright and shrunk at 1 to 89
 * degrees, so only those routines are here. Any other combination stops the program.
 *
 * The arithmetic keeps the original's register widths: 8.8 fixed-point steps that advance when
 * adding a fraction carries, byte-sized multiplies (only the low byte of a frame extent counts)
 * and byte-sized counters, so every pixel lands where it did.
 */

#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"
#include "scalfram.h"

namespace Intro {

#define FULL_SIZE       256

/* ClipEdges: the clip box edges a turned span crosses. */
#define CLIP_LEFT       1
#define CLIP_TOP        2
#define CLIP_RIGHT      4
#define CLIP_BOTTOM     8

/* The first 90 entries of the sine and cosine tables, in 256ths with 255 for 1. */
static const uint8_t CosTable[90] = {
	255, 255, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 250, 248, 247, 246, 245,
	243, 242, 241, 239, 237, 236, 234, 232, 230, 228, 226, 224, 221, 219, 217, 215, 212, 210,
	207, 204, 202, 198, 196, 193, 190, 187, 184, 181, 178, 175, 171, 168, 165, 161, 158, 154,
	150, 147, 143, 139, 136, 131, 128, 124, 120, 116, 112, 108, 104, 100, 96, 92, 88, 83,
	79, 74, 71, 66, 62, 58, 53, 49, 45, 40, 36, 31, 27, 22, 18, 13, 9, 4
};
static const uint8_t SinTable[90] = {
	0, 4, 9, 13, 18, 22, 27, 31, 36, 40, 45, 49, 53, 58, 62, 66, 71, 75,
	79, 83, 88, 92, 96, 100, 104, 108, 112, 116, 120, 124, 128, 131, 136, 139, 143, 147,
	150, 154, 158, 161, 165, 168, 171, 175, 178, 181, 184, 187, 190, 193, 196, 198, 202, 204,
	207, 210, 212, 215, 217, 219, 221, 224, 226, 228, 230, 232, 234, 236, 237, 239, 241, 242,
	243, 245, 246, 247, 248, 250, 250, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255, 255
};

/* The frame being drawn: its extents around the hot spot, and the frame row being walked. */
static int16_t FrameRight, FrameLeft, FrameTop, FrameBottom;
static uint16_t FrameWidth, FrameHeight;
static int16_t SpanRow;

/* The scaled frame's box on the view, and where drawing starts. */
static int16_t DrawLeft, DrawTop, DrawRight, DrawBottom;
static int16_t ScreenX, ScreenY;
static uint16_t ScaledWidth, ScaledHeight;
static uint8_t XFraction, YFraction;
static uint16_t CurrentScale;

/* Turned drawing: the scaled cosine and sine, and how far a clipped span may run. */
static uint8_t ScaledCos, ScaledSin;
static uint16_t CosStep, SinStep;
static uint16_t StepFraction;
static uint16_t FillGaps;               /* kept from one turned frame to the next */
static uint8_t ClipEdges;
static int16_t ColumnsLeft, RowsLeft;
static uint16_t RunLength;
static uint8_t RowRepeat;

static int16_t ClipLeft, ClipTop, ClipRight, ClipBottom;
static int32_t RowTable;

/* A run-encoded span is unpacked here before a clipped or turned draw. The row pitch sits just
 * before it: the turned fill loop can read a byte or two back from the buffer's start. */
static struct {
	uint16_t rowPitch;
	uint8_t unused;
	uint8_t pixels[320];
} Span;

static uint16_t Get16(const uint8_t *p) { return (uint16_t) (p[0] | p[1] << 8); }
static int16_t GetSigned16(const uint8_t *p) { return (int16_t) Get16(p); }

/* a += b in a byte; true when it carries. */
static bool AddCarry(uint8_t *a, uint8_t b)
{
	unsigned sum = *a + b;

	*a = (uint8_t) sum;
	return sum > 0xff;
}

/* a -= b in a byte; true when it borrows. */
static bool SubBorrow(uint8_t *a, uint8_t b)
{
	bool borrow = *a < b;

	*a = (uint8_t) (*a - b);
	return borrow;
}

/* The original trusted the frame's box and could write through rows past its view's table; stop
 * instead. */
static uint8_t *RowPointer(int16_t y)
{
	if (y < 0 || y >= SCREEN_HEIGHT)
		plat_fatal("DrawScaledFrame: a row is off the screen.");
	return LINEAR(LinearGet32(RowTable + (int32_t) y * 4));
}

/* The processor faulted when a quotient did not fit its register. */
static void CheckQuotient(uint32_t quotient, uint32_t limit)
{
	if (quotient > limit)
		plat_fatal("Divide error");
}

static const uint8_t *SkipSpan(const uint8_t *span)
{
	uint16_t header = Get16(span);
	int16_t left = (int16_t) (header >> 1);
	const uint8_t *p = span + 6;

	if ((header & 1) == 0)
		return p + left;
	do {
		uint8_t run = *p;
		int16_t count = run >> 1;

		p += 2;
		if ((run & 1) == 0)
			p += count - 1;
		left -= count;
	} while (left != 0);
	return p;
}

/* Unpacks count run-encoded pixels into the span buffer; returns the source just past them. */
static const uint8_t *UnpackSpan(const uint8_t *p, uint16_t count)
{
	uint8_t *out = Span.pixels;

	do {
		uint8_t run = *p++;
		uint16_t n = run >> 1;

		if (run & 1)
			memset(out, *p++, n);
		else {
			memcpy(out, p, n);
			p += n;
		}
		out += n;
		count -= n;
	} while (count != 0);
	return p;
}

static bool FrameNeedsClip(void)
{
	return DrawLeft < ClipLeft || DrawRight > ClipRight || DrawTop < ClipTop || DrawBottom > ClipBottom;
}

/* ---- Shrunk, upright ---- */

/* The box of the frame shrunk about its hot spot; false when it misses the clip box. */
static bool PlaceShrunk(int16_t x, int16_t y, uint8_t scale)
{
	uint16_t product;

	product = (uint8_t) FrameLeft * scale;
	XFraction = (uint8_t) -(uint8_t) product;
	ScreenX = DrawLeft = (int16_t) (x - (product >> 8));
	if (DrawLeft > ClipRight)
		return false;
	product = (uint8_t) FrameTop * scale;
	YFraction = (uint8_t) -(uint8_t) product;
	ScreenY = DrawTop = (int16_t) (y - (product >> 8));
	if (DrawTop > ClipBottom)
		return false;
	product = (uint16_t) ((uint8_t) FrameWidth * scale + XFraction);
	ScaledWidth = (uint16_t) ((product >> 8) + 1);
	DrawRight = (int16_t) (ScaledWidth + ScreenX);
	if (DrawRight < ClipLeft)
		return false;
	product = (uint16_t) ((uint8_t) FrameHeight * scale + YFraction);
	ScaledHeight = (uint16_t) ((product >> 8) + 1);
	DrawBottom = (int16_t) (ScaledHeight + ScreenY);
	if (DrawBottom < ClipTop)
		return false;
	return true;
}

/* A pixel is stored only when adding the scale to the fraction carries. */
static const uint8_t *ShrunkSpan(const uint8_t *span, int16_t row, uint8_t scale)
{
	uint16_t header = Get16(span);
	uint16_t product = (uint8_t) (GetSigned16(span + 2) + FrameLeft) * scale;
	uint8_t fraction = (uint8_t) product;
	uint8_t column = (uint8_t) ((product >> 8) + AddCarry(&fraction, XFraction));
	uint8_t *out = RowPointer(row) + (int16_t) (column + ScreenX);
	const uint8_t *p = span + 6;
	uint8_t left = (uint8_t) (header >> 1);

	if ((header & 1) == 0) {
		for (; left != 0; left--) {
			uint8_t pixel = *p++;

			if (AddCarry(&fraction, scale))
				*out++ = pixel;
		}
		return p;
	}
	do {
		uint8_t run = *p++;
		uint8_t count = run >> 1;

		left -= count;
		if (run & 1) {
			uint8_t pixel = *p++;

			do {
				if (AddCarry(&fraction, scale))
					*out++ = pixel;
			} while (--count != 0);
		} else {
			do {
				uint8_t pixel = *p++;

				if (AddCarry(&fraction, scale))
					*out++ = pixel;
			} while (--count != 0);
		}
	} while (left != 0);
	return p;
}

/* A quotient of a division by the scale, rounded up; the division is a byte one. */
static uint8_t ShrunkPixels(uint8_t whole, uint8_t fraction, uint8_t scale, bool *past)
{
	uint16_t room = (uint16_t) (whole << 8);
	uint16_t quotient;

	*past = room < fraction;
	if (*past)
		return 0;
	room -= fraction;
	quotient = room / scale;
	CheckQuotient(quotient, 0xff);
	if (room % scale != 0)
		quotient++;
	return (uint8_t) quotient;
}

static const uint8_t *ClippedShrunkSpan(const uint8_t *span, int16_t row, uint8_t scale)
{
	uint16_t header = Get16(span);
	int16_t spanX = GetSigned16(span + 2);
	const uint8_t *pixels = span + 6;
	const uint8_t *next;
	uint16_t product;
	uint8_t fraction, low, count;
	int16_t column, end, offset;
	bool past;

	RunLength = header >> 1;
	if (header & 1) {
		next = UnpackSpan(pixels, RunLength);
		pixels = Span.pixels;
	} else
		next = pixels + RunLength;
	product = (uint8_t) (spanX + FrameLeft) * scale;
	fraction = (uint8_t) product;
	column = (int16_t) ((uint8_t) ((product >> 8) + AddCarry(&fraction, XFraction)) + ScreenX);
	if (column > ClipRight)
		return next;
	product = (uint8_t) RunLength * scale;
	low = (uint8_t) product;
	end = (int16_t) ((uint8_t) ((product >> 8) + AddCarry(&low, fraction)) + column);
	if (end < ClipLeft)
		return next;
	if (end > ClipRight) {
		/* count only the pixels up to the right edge */
		count = ShrunkPixels((uint8_t) (ClipRight + 1 - column), fraction, scale, &past);
		if (past)
			return next;
		RunLength = (uint16_t) ((RunLength & 0xff00) | count);
	}
	offset = column - ClipLeft;
	if (offset < 0) {
		/* skip the pixels left of the left edge */
		uint8_t skip = ShrunkPixels((uint8_t) -offset, fraction, scale, &past);

		if (past)
			return next;
		if (RunLength <= skip)
			return next;
		RunLength -= skip;
		pixels += skip;
		product = skip * scale;
		column += (uint8_t) ((product >> 8) + AddCarry(&fraction, (uint8_t) product));
	}
	uint8_t *out = RowPointer(row) + column;

	count = (uint8_t) RunLength;
	do {
		uint8_t pixel = *pixels++;

		if (AddCarry(&fraction, scale))
			*out++ = pixel;
	} while (--count != 0);
	return next;
}

/* A frame row reaches the screen only when adding the scale to the row fraction carries. */
static void ShrunkRows(const uint8_t *span, uint8_t scale)
{
	int16_t row = ScreenY;
	uint8_t fraction = YFraction;
	uint8_t rowsLeft = (uint8_t) FrameHeight;

	for (;;) {
		if (!AddCarry(&fraction, scale)) {
			SpanRow++;
			if (--rowsLeft == 0)
				return;
			continue;
		}
		for (;;) {
			uint16_t header = Get16(span);
			int16_t spanY = GetSigned16(span + 4);

			if (header == 0)
				return;
			if (spanY == SpanRow)
				span = ShrunkSpan(span, row, scale);
			else if (spanY > SpanRow)
				break;
			else
				span = SkipSpan(span);
		}
		SpanRow++;
		row++;
		if (--rowsLeft == 0)
			return;
	}
}

static void ClippedShrunkRows(const uint8_t *span, uint8_t scale)
{
	uint8_t fraction = YFraction;
	uint8_t rowsLeft = (uint8_t) FrameHeight;

	if (ScreenY < ClipTop) {
		for (;;) {
			SpanRow++;
			if (AddCarry(&fraction, scale)) {
				ScreenY++;
				if (ScreenY >= ClipTop)
					break;
			}
			if (--rowsLeft == 0)
				return;
		}
	}
	for (;;) {
		if (!AddCarry(&fraction, scale)) {
			SpanRow++;
			if (--rowsLeft == 0)
				return;
			continue;
		}
		for (;;) {
			uint16_t header = Get16(span);
			int16_t spanY = GetSigned16(span + 4);

			if (header == 0)
				return;
			if (spanY == SpanRow)
				span = ClippedShrunkSpan(span, ScreenY, scale);
			else if (spanY > SpanRow)
				break;
			else
				span = SkipSpan(span);
		}
		SpanRow++;
		ScreenY++;
		if (ScreenY > ClipBottom)
			return;
		if (--rowsLeft == 0)
			return;
	}
}

static void ShrinkFrame(int16_t x, int16_t y, const uint8_t *spans)
{
	uint8_t scale = (uint8_t) CurrentScale;

	if (!PlaceShrunk(x, y, scale))
		return;
	if (FrameNeedsClip())
		ClippedShrunkRows(spans, scale);
	else
		ShrunkRows(spans, scale);
}

/* ---- Shrunk and turned 1 to 89 degrees ----
 *
 * Frame rows run along the turned column axis and spans along the turned row axis. A span's
 * position steps a column when its column fraction carries and a row when its row fraction does;
 * a step both ways at once leaves a hole, which the fill loops plug with the previous pixel.
 */

/* Scales the cosine and sine and sizes the turned frame's box. */
static void TurnSteps(uint8_t scale, uint8_t cosine, uint8_t sine)
{
	uint16_t sum;

	ScaledCos = (uint8_t) ((cosine * scale) >> 8);
	CosStep = ScaledCos != 0 ? ScaledCos : 1;
	ScaledSin = (uint8_t) ((sine * scale) >> 8);
	SinStep = ScaledSin != 0 ? ScaledSin : 1;
	sum = (uint16_t) ((uint8_t) FrameHeight * ScaledCos + (uint8_t) FrameWidth * ScaledSin);
	ScaledHeight = (uint16_t) ((uint8_t) ((sum >> 8) + ((sum & 0xff) != 0)) + 1);
	sum = (uint16_t) ((uint8_t) FrameHeight * ScaledSin + (uint8_t) FrameWidth * ScaledCos);
	ScaledWidth = (uint16_t) ((uint8_t) ((sum >> 8) + ((sum & 0xff) != 0)) + 1);
}

/* The box around the turned frame; the first row starts at the hot spot less the turned extents. */
static bool PlaceTurned(int16_t x, int16_t y, uint8_t scale, uint8_t cosine, uint8_t sine,
	int16_t *startX, int16_t *startY)
{
	TurnSteps(scale, cosine, sine);
	cosine = ScaledCos;
	sine = ScaledSin;
	x += ((uint8_t) FrameTop * sine) >> 8;
	y -= ((uint8_t) FrameTop * cosine) >> 8;
	x -= ((uint8_t) FrameLeft * cosine) >> 8;
	y -= ((uint8_t) FrameLeft * sine) >> 8;
	DrawTop = y;
	if (DrawTop > ClipBottom)
		return false;
	DrawBottom = (int16_t) (ScaledHeight + y);
	if (DrawBottom < ClipTop)
		return false;
	DrawLeft = (int16_t) (x - ((((uint8_t) FrameHeight * sine) >> 8) + 1));
	if (DrawLeft > ClipRight)
		return false;
	DrawRight = (int16_t) (DrawLeft + ScaledWidth);
	if (DrawRight < ClipLeft)
		return false;
	*startX = x;
	*startY = y;
	return true;
}

static void TurnedLoop(const uint8_t *pixels, uint8_t *out, uint8_t count, uint8_t columnFraction,
	uint8_t rowFraction)
{
	uint8_t cosine = ScaledCos, sine = ScaledSin;

	for (;;) {
		*out = *pixels++;
		if (--count == 0)
			return;
		for (;;) {
			if (AddCarry(&columnFraction, cosine)) {
				out++;
				if (AddCarry(&rowFraction, sine))
					out += Span.rowPitch;
				break;
			}
			if (AddCarry(&rowFraction, sine)) {
				out += Span.rowPitch;
				break;
			}
			pixels++;
			if (--count == 0)
				return;
		}
	}
}

static void TurnedFillLoop(const uint8_t *pixels, uint8_t *out, uint8_t count, uint8_t columnFraction,
	uint8_t rowFraction)
{
	uint8_t cosine = ScaledCos, sine = ScaledSin;
	uint8_t last;

	for (;;) {
		last = *pixels++;
		*out = last;
		if (--count == 0)
			return;
		for (;;) {
			if (AddCarry(&columnFraction, cosine)) {
				out++;
				if (AddCarry(&rowFraction, sine)) {
					*out = last;
					out += Span.rowPitch;
				}
				break;
			}
			if (AddCarry(&rowFraction, sine)) {
				out += Span.rowPitch;
				break;
			}
			last = *pixels++;
			if (--count == 0)
				return;
		}
	}
}

/* As TurnedLoop, stopping at the far clip edges. */
static void ClippedTurnedLoop(const uint8_t *pixels, uint8_t *out, uint8_t count, uint8_t columnFraction,
	uint8_t rowFraction)
{
	uint8_t cosine = ScaledCos, sine = ScaledSin;

	if ((ClipEdges & (CLIP_RIGHT | CLIP_BOTTOM)) == 0) {
		TurnedLoop(pixels, out, count, columnFraction, rowFraction);
		return;
	}
	for (;;) {
		*out = *pixels++;
		if (--count == 0)
			return;
		for (;;) {
			if (AddCarry(&columnFraction, cosine)) {
				if (--ColumnsLeft <= 0)
					return;
				out++;
				if (AddCarry(&rowFraction, sine)) {
					if (--RowsLeft <= 0)
						return;
					out += Span.rowPitch;
				}
				break;
			}
			if (AddCarry(&rowFraction, sine)) {
				if (--RowsLeft <= 0)
					return;
				out += Span.rowPitch;
				break;
			}
			pixels++;
			if (--count == 0)
				return;
		}
	}
}

/* As TurnedFillLoop, clipped. A span cut at the left first plugs the corner before its first
 * visible pixel, unless it starts on the top row. */
static void ClippedTurnedFillLoop(const uint8_t *pixels, uint8_t *out, uint8_t count, uint8_t columnFraction,
	uint8_t rowFraction, int16_t row)
{
	uint8_t cosine = ScaledCos, sine = ScaledSin;
	uint8_t last;

	if (ClipEdges & CLIP_LEFT) {
		if (row != ClipTop) {
			const uint8_t *back = pixels;
			uint8_t backColumn = columnFraction, backRow = rowFraction;

			for (;;) {
				back--;
				if (!SubBorrow(&backColumn, cosine)) {
					if (!SubBorrow(&backRow, sine))
						continue;
					break;
				}
				if (SubBorrow(&backRow, sine))
					*(out - Span.rowPitch) = *back;
				break;
			}
		}
	} else if ((ClipEdges & (CLIP_RIGHT | CLIP_BOTTOM)) == 0) {
		TurnedFillLoop(pixels, out, count, columnFraction, rowFraction);
		return;
	}
	for (;;) {
		last = *pixels++;
		*out = last;
		if (--count == 0)
			return;
		for (;;) {
			if (AddCarry(&columnFraction, cosine)) {
				if (--ColumnsLeft <= 0)
					return;
				out++;
				if (AddCarry(&rowFraction, sine)) {
					*out = last;
					out += Span.rowPitch;
					if (--RowsLeft <= 0)
						return;
				}
				break;
			}
			if (AddCarry(&rowFraction, sine)) {
				out += Span.rowPitch;
				if (--RowsLeft <= 0)
					return;
				break;
			}
			last = *pixels++;
			if (--count == 0)
				return;
		}
	}
}

static const uint8_t *TurnedSpan(const uint8_t *span)
{
	uint16_t header = Get16(span);
	uint16_t along = (uint16_t) (GetSigned16(span + 2) + FrameLeft);
	uint16_t count = header >> 1;
	const uint8_t *pixels = span + 6;
	const uint8_t *next = pixels + count;
	uint16_t product;
	uint8_t columnFraction, rowFraction;
	int16_t column, row;

	product = (uint8_t) along * ScaledCos;
	columnFraction = (uint8_t) product;
	column = (int16_t) (ScreenX + (product >> 8));
	product = (uint8_t) along * ScaledSin;
	rowFraction = (uint8_t) product;
	row = (int16_t) (ScreenY + (product >> 8));
	if (header & 1) {
		next = UnpackSpan(pixels, count);
		pixels = Span.pixels;
	}
	if (FillGaps)
		TurnedFillLoop(pixels, RowPointer(row) + column, (uint8_t) count, columnFraction, rowFraction);
	else
		TurnedLoop(pixels, RowPointer(row) + column, (uint8_t) count, columnFraction, rowFraction);
	return next;
}

/* How many pixels to skip to reach a clip edge distance away, rounded up. */
static uint16_t TurnedSkip(int16_t distance, uint8_t fraction, uint16_t step)
{
	uint32_t room = ((uint32_t) (uint16_t) distance << 8) - fraction;
	uint32_t quotient = room / step;

	CheckQuotient(quotient, 0xffff);
	if (room % step != 0)
		quotient++;
	return (uint16_t) quotient;
}

/* Moves a position on by the scaled step times the low byte of count. */
static void StepAlong(uint16_t count, uint8_t step, uint8_t *fraction, int16_t *position)
{
	uint16_t product = (uint8_t) count * step;

	*position += (uint8_t) ((product >> 8) + AddCarry(fraction, (uint8_t) product));
}

static const uint8_t *ClippedTurnedSpan(const uint8_t *span)
{
	uint16_t header = Get16(span);
	uint16_t along = (uint16_t) (GetSigned16(span + 2) + FrameLeft);
	const uint8_t *pixels = span + 6;
	const uint8_t *next;
	uint16_t product, skip;
	int16_t column = ScreenX, row = ScreenY, end, room, offset;
	uint8_t low;

	RunLength = header >> 1;
	if (header & 1) {
		next = UnpackSpan(pixels, RunLength);
		pixels = Span.pixels;
	} else
		next = pixels + RunLength;
	ClipEdges = 0;
	product = (uint8_t) along * ScaledCos;
	XFraction = (uint8_t) product;
	column += product >> 8;
	product = (uint8_t) along * ScaledSin;
	YFraction = (uint8_t) product;
	row += product >> 8;
	if (column > ClipRight)
		return next;
	offset = column - ClipLeft;
	if (offset < 0) {
		ClipEdges |= CLIP_LEFT;
		skip = TurnedSkip(-offset, XFraction, CosStep);
		pixels += skip;
		if ((int16_t) RunLength <= (int16_t) skip)
			return next;
		RunLength -= skip;
		StepAlong(skip, ScaledCos, &XFraction, &column);
		StepAlong(skip, ScaledSin, &YFraction, &row);
	}
	if (row > ClipBottom)
		return next;
	offset = row - ClipTop;
	if (offset < 0) {
		skip = TurnedSkip(-offset, YFraction, SinStep);
		pixels += skip;
		if ((int16_t) RunLength <= (int16_t) skip)
			return next;
		RunLength -= skip;
		StepAlong(skip, ScaledSin, &YFraction, &row);
		StepAlong(skip, ScaledCos, &XFraction, &column);
		ClipEdges |= CLIP_TOP;
	}
	room = (int16_t) (ClipRight + 1 - column);
	if (room <= 0)
		return next;
	ColumnsLeft = room;
	product = (uint8_t) RunLength * ScaledCos;
	low = (uint8_t) product;
	end = (int16_t) ((uint8_t) ((product >> 8) + AddCarry(&low, XFraction)) + column);
	if (end < ClipLeft)
		return next;
	if (end > ClipRight)
		ClipEdges |= CLIP_RIGHT;
	room = (int16_t) (ClipBottom + 1 - row);
	if (room <= 0)
		return next;
	RowsLeft = room;
	product = (uint8_t) RunLength * ScaledSin;
	low = (uint8_t) product;
	end = (int16_t) ((uint8_t) ((product >> 8) + AddCarry(&low, YFraction)) + row);
	if (end < ClipTop)
		return next;
	if (end > ClipBottom)
		ClipEdges |= CLIP_BOTTOM;
	if ((uint8_t) RunLength == 0)
		return next;
	if (FillGaps)
		ClippedTurnedFillLoop(pixels, RowPointer(row) + column, (uint8_t) RunLength, XFraction, YFraction, row);
	else
		ClippedTurnedLoop(pixels, RowPointer(row) + column, (uint8_t) RunLength, XFraction, YFraction);
	return next;
}

/* Walks the frame's rows; each starts one turned column step on from the last. */
static void TurnedRows(const uint8_t *span, int16_t startX, int16_t startY, bool clipped)
{
	uint8_t cosine = ScaledCos, sine = ScaledSin;
	uint8_t columnFraction = 0, rowFraction = 0;

	for (;;) {
		StepFraction = (uint16_t) (columnFraction | rowFraction << 8);
		ScreenX = startX;
		ScreenY = startY;
		for (;;) {
			uint16_t header = Get16(span);
			int16_t spanY = GetSigned16(span + 4);

			if (header == 0)
				return;
			if (spanY == SpanRow)
				span = clipped ? ClippedTurnedSpan(span) : TurnedSpan(span);
			else if (spanY < SpanRow)
				span = SkipSpan(span);
			else
				break;
		}
		startX = ScreenX;
		startY = ScreenY;
		columnFraction = (uint8_t) StepFraction;
		rowFraction = (uint8_t) (StepFraction >> 8);
		for (;;) {
			uint16_t gaps = 0;

			SpanRow++;
			if (--FrameHeight == 0)
				return;
			if (AddCarry(&columnFraction, sine)) {
				startX--;
				/* the original compares these as signed bytes */
				if ((int8_t) cosine >= (int8_t) sine)
					gaps = 2;
				if (!AddCarry(&rowFraction, cosine)) {
					FillGaps = gaps;
					break;
				}
			} else if (!AddCarry(&rowFraction, cosine))
				continue;
			startY++;
			if ((int8_t) cosine < (int8_t) sine)
				gaps = 2;
			FillGaps = gaps;
			break;
		}
	}
}

static void ShrinkTurned(int16_t x, int16_t y, const uint8_t *spans, uint8_t cosine, uint8_t sine)
{
	int16_t startX, startY;

	if (!PlaceTurned(x, y, (uint8_t) CurrentScale, cosine, sine, &startX, &startY))
		return;
	TurnedRows(spans, startX, startY, FrameNeedsClip());
}

/* ---- Enlarged, upright ----
 *
 * Each source pixel is as wide as the scale's whole part plus what the column fraction carries;
 * each frame row is copied down as many screen rows as the row fraction gives it.
 */

static bool PlaceEnlarged(int16_t x, int16_t y, uint16_t scale)
{
	uint32_t product;

	product = (uint32_t) (uint16_t) FrameLeft * scale;
	XFraction = (uint8_t) -(uint8_t) product;
	ScreenX = DrawLeft = (int16_t) (x - (uint16_t) (product >> 8));
	if (DrawLeft > ClipRight)
		return false;
	product = (uint32_t) (uint16_t) FrameTop * scale;
	YFraction = (uint8_t) -(uint8_t) product;
	ScreenY = DrawTop = (int16_t) (y - (uint16_t) (product >> 8));
	if (DrawTop > ClipBottom)
		return false;
	product = (uint32_t) FrameWidth * scale + XFraction;
	if ((int16_t) (uint16_t) (product >> 8) <= 0)
		return false;
	DrawRight = (int16_t) ((uint16_t) (product >> 8) - 1 + ScreenX);
	if (DrawRight < ClipLeft)
		return false;
	product = (uint32_t) FrameHeight * scale + YFraction;
	if ((int16_t) (uint16_t) (product >> 8) <= 0)
		return false;
	DrawBottom = (int16_t) ((uint16_t) (product >> 8) - 1 + ScreenY);
	if (DrawBottom < ClipTop)
		return false;
	return true;
}

static const uint8_t *EnlargedSpan(const uint8_t *span, int16_t row, uint16_t scale)
{
	uint16_t header = Get16(span);
	uint32_t product = (uint32_t) (uint16_t) (GetSigned16(span + 2) + FrameLeft) * scale + XFraction;
	uint8_t fraction = (uint8_t) product;
	int16_t start = (int16_t) ((uint16_t) (product >> 8) + ScreenX);
	uint8_t *out = RowPointer(row) + start;
	const uint8_t *p = span + 6;
	uint16_t drawn = 0;
	uint8_t left = (uint8_t) (header >> 1);
	uint8_t repeats;

	if ((header & 1) == 0) {
		do {
			uint8_t pixel = *p++;
			uint8_t width = (uint8_t) ((scale >> 8) + AddCarry(&fraction, (uint8_t) scale));

			drawn += width;
			memset(out, pixel, width);
			out += width;
		} while (--left != 0);
	} else {
		do {
			uint8_t run = *p++;
			uint8_t count = run >> 1;

			left -= count;
			if (run & 1) {
				uint32_t fill = (uint32_t) count * CurrentScale + fraction;
				uint16_t width = (uint16_t) (fill >> 8);

				fraction = (uint8_t) fill;
				drawn += width;
				memset(out, *p++, width);
				out += width;
			} else {
				do {
					uint8_t pixel = *p++;
					uint16_t width = (uint16_t) ((CurrentScale >> 8)
						+ AddCarry(&fraction, (uint8_t) CurrentScale));

					drawn += width;
					memset(out, pixel, width);
					out += width;
				} while (--count != 0);
			}
		} while (left != 0);
	}
	/* the rows below repeat this one */
	repeats = (uint8_t) (RowRepeat - 1);
	if (repeats != 0) {
		do {
			uint8_t *from = RowPointer(row) + start;

			row++;
			memmove(RowPointer(row) + start, from, drawn);
		} while (--repeats != 0);
	}
	return p;
}

static const uint8_t *ClippedEnlargedSpan(const uint8_t *span, uint16_t scale)
{
	uint16_t header = Get16(span);
	int16_t spanX = GetSigned16(span + 2);
	const uint8_t *pixels = span + 6;
	const uint8_t *next;
	uint8_t cutEnd = 0;
	uint32_t product, room;
	uint8_t fraction, width, count, repeats;
	int16_t start, end, column, offset;
	uint16_t drawn = 0;
	uint8_t *out, *rowStart;

	RunLength = header >> 1;
	if (header & 1) {
		next = UnpackSpan(pixels, RunLength);
		pixels = Span.pixels;
	} else
		next = pixels + RunLength;
	product = (uint32_t) (uint16_t) (spanX + FrameLeft) * scale + XFraction;
	fraction = (uint8_t) product;
	start = (int16_t) ((uint16_t) (product >> 8) + ScreenX);
	if (start > ClipRight)
		return next;
	product = (uint32_t) RunLength * scale + fraction;
	end = (int16_t) ((uint16_t) (product >> 8) + start);
	if (end < ClipLeft)
		return next;
	if (end > ClipRight) {
		/* count only the pixels up to the right edge; the last is cut */
		room = ((uint32_t) (uint16_t) (ClipRight + 1 - start) << 8) - fraction;
		CheckQuotient(room / scale, 0xffff);
		if (room % scale != 0)
			cutEnd = 1;
		RunLength = (uint16_t) (room / scale);
	}
	offset = start - ClipLeft;
	if (offset < 0) {
		/* skip the pixels left of the left edge */
		uint16_t skip;

		room = (uint32_t) (uint16_t) -offset << 8;
		if (room < fraction)
			return next;
		room -= fraction;
		skip = (uint16_t) (room / scale);
		if (RunLength < skip)
			return next;
		RunLength -= skip;
		pixels += skip;
		product = (uint32_t) skip * scale + fraction;
		fraction = (uint8_t) product;
		column = ClipLeft;
		width = (uint8_t) ((uint16_t) (product >> 8) + start - ClipLeft);
		width = (uint8_t) (width + (scale >> 8) + AddCarry(&fraction, (uint8_t) scale));
	} else {
		column = start;
		width = (uint8_t) ((scale >> 8) + AddCarry(&fraction, (uint8_t) scale));
	}
	out = rowStart = RowPointer(ScreenY) + column;
	count = (uint8_t) RunLength;
	if (count != 0) {
		do {
			uint8_t pixel = *pixels++;

			drawn += width;
			memset(out, pixel, width);
			out += width;
			width = (uint8_t) ((scale >> 8) + AddCarry(&fraction, (uint8_t) scale));
		} while (--count != 0);
	}
	if (cutEnd) {
		/* widen the cut last pixel to reach the right edge */
		int16_t gap = (int16_t) (column + drawn - ClipRight - 1);

		if (gap < 0)
			gap = -gap;
		drawn += gap;
		memset(out, *pixels, (uint16_t) gap);
	}
	repeats = (uint8_t) (RowRepeat - 1);
	if (repeats != 0) {
		int16_t y = ScreenY;

		do {
			if (++y > ClipBottom)
				break;
			memmove(rowStart + Span.rowPitch, rowStart, drawn);
			rowStart += Span.rowPitch;
		} while (--repeats != 0);
	}
	return next;
}

static void EnlargedRows(const uint8_t *span, uint16_t scale)
{
	int16_t row = ScreenY;

	for (;;) {
		RowRepeat = (uint8_t) ((scale >> 8) + AddCarry(&YFraction, (uint8_t) scale));
		for (;;) {
			uint16_t header = Get16(span);
			int16_t spanY = GetSigned16(span + 4);

			if (header == 0)
				return;
			if (spanY == SpanRow)
				span = EnlargedSpan(span, row, scale);
			else if (spanY < SpanRow)
				span = SkipSpan(span);
			else
				break;
		}
		SpanRow++;
		if (--FrameHeight == 0)
			return;
		row += RowRepeat;
	}
}

static void ClippedEnlargedRows(const uint8_t *span, uint16_t scale)
{
	bool cutAtTop = false;

	if (ScreenY < ClipTop) {
		int16_t y = ScreenY;

		for (;;) {
			y += (uint8_t) ((scale >> 8) + AddCarry(&YFraction, (uint8_t) scale));
			if (y > ClipTop)
				break;
			SpanRow++;
			if (--FrameHeight == 0)
				return;
		}
		ScreenY = ClipTop;
		RowRepeat = (uint8_t) (y - ClipTop);
		cutAtTop = true;
	}
	for (;;) {
		if (!cutAtTop)
			RowRepeat = (uint8_t) ((scale >> 8) + AddCarry(&YFraction, (uint8_t) scale));
		cutAtTop = false;
		for (;;) {
			uint16_t header = Get16(span);
			int16_t spanY = GetSigned16(span + 4);

			if (header == 0)
				return;
			if (spanY == SpanRow)
				span = ClippedEnlargedSpan(span, scale);
			else if (spanY < SpanRow)
				span = SkipSpan(span);
			else
				break;
		}
		SpanRow++;
		if (--FrameHeight == 0)
			return;
		ScreenY += RowRepeat;
		if (ScreenY > ClipBottom)
			return;
	}
}

static void EnlargeFrame(int16_t x, int16_t y, const uint8_t *spans)
{
	uint16_t scale = CurrentScale;

	if (!PlaceEnlarged(x, y, scale))
		return;
	if (FrameNeedsClip())
		ClippedEnlargedRows(spans, scale);
	else
		EnlargedRows(spans, scale);
}

/* ---- Entry ---- */

void DrawScaledFrame(View *view, int16_t x, int16_t y, int32_t shape, int16_t frame, int16_t angle,
	int16_t scale, uint8_t flip)
{
	uint16_t entry;
	const uint8_t *p;

	if (scale == FULL_SIZE && ((uint8_t) angle | flip) == 0) {
		DrawFrame(view, x, y, shape, frame, 0);
		return;
	}
	ClipLeft = view->clip.x0;
	ClipTop = view->clip.y0;
	ClipRight = view->clip.x1;
	ClipBottom = view->clip.y1;
	RowTable = view->rowTable;
	Span.rowPitch = (uint16_t) (LinearGet32(RowTable + (ClipTop + 1) * 4) - LinearGet32(RowTable + ClipTop * 4));
	entry = (uint16_t) ((frame + 1) * 4);
	if (LinearGet16(shape + 4) <= entry)
		return;
	/* a real-mode far pointer kept only 20 bits of the offset */
	p = LINEAR(shape + (int32_t) (LinearGet32(shape + entry) & 0xfffff));
	FrameRight = GetSigned16(p);
	FrameLeft = GetSigned16(p + 2);
	FrameWidth = (uint16_t) (FrameRight + FrameLeft + 1);
	FrameTop = GetSigned16(p + 4);
	SpanRow = (int16_t) -FrameTop;
	FrameBottom = GetSigned16(p + 6);
	FrameHeight = (uint16_t) (FrameBottom + FrameTop + 1);
	p += 8;
	if (angle < 0)
		angle += 360;
	CurrentScale = (uint16_t) scale;
	if (flip == 0 && angle == 0) {
		if (CurrentScale >= FULL_SIZE)
			EnlargeFrame(x, y, p);
		else
			ShrinkFrame(x, y, p);
	} else if (flip == 0 && angle > 0 && angle < 90 && CurrentScale < FULL_SIZE)
		ShrinkTurned(x, y, p, CosTable[angle], SinTable[angle]);
	else
		plat_fatal("DrawScaledFrame: this scale, angle and flip are not supported.");
}

}

extern "C" void ResetIntroScalframGlobals(void)
{
	Intro::FrameRight = 0;
	Intro::FrameLeft = 0;
	Intro::FrameTop = 0;
	Intro::FrameBottom = 0;
	Intro::FrameWidth = 0;
	Intro::FrameHeight = 0;
	Intro::SpanRow = 0;
	Intro::DrawLeft = 0;
	Intro::DrawTop = 0;
	Intro::DrawRight = 0;
	Intro::DrawBottom = 0;
	Intro::ScreenX = 0;
	Intro::ScreenY = 0;
	Intro::ScaledWidth = 0;
	Intro::ScaledHeight = 0;
	Intro::XFraction = 0;
	Intro::YFraction = 0;
	Intro::CurrentScale = 0;
	Intro::ScaledCos = 0;
	Intro::ScaledSin = 0;
	Intro::CosStep = 0;
	Intro::SinStep = 0;
	Intro::StepFraction = 0;
	Intro::FillGaps = 0;
	Intro::ClipEdges = 0;
	Intro::ColumnsLeft = 0;
	Intro::RowsLeft = 0;
	Intro::RunLength = 0;
	Intro::RowRepeat = 0;
	Intro::ClipLeft = 0;
	Intro::ClipTop = 0;
	Intro::ClipRight = 0;
	Intro::ClipBottom = 0;
	Intro::RowTable = 0;
	memset(&Intro::Span, 0, sizeof Intro::Span);
}
