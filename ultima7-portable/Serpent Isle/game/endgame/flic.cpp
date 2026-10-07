/* Black Gate ENDGAME.EXE: flic.c, with flicbrun.asm, flicdlta.asm and flicclr.asm. */

#include "u7port.h"
#include "arena.h"
#include "view.h"
#include "iff.h"
#include "palette.h"
#include "endview.h"
#include "flic.h"

namespace Endgame {

/* Chunk types inside a frame. */
#define FLI_COLOR       11
#define FLI_LC          12
#define FLI_BLACK       13
#define FLI_BRUN        15
#define FLI_COPY        16

#define LINE_BYTES      320

static inline uint16_t Word(const uint8_t *p) { return (uint16_t) (p[0] | p[1] << 8); }
static inline uint32_t Long(const uint8_t *p) { return Word(p) | (uint32_t) Word(p + 2) << 16; }

/* Every line is a count of packets, each a run of one byte repeated or, when its count is
 * negative, bytes copied as they are. */
static void DecodeBrun(const uint8_t *data, uint8_t *screen, int16_t lines)
{
	for (; lines > 0; lines--, screen += LINE_BYTES) {
		uint8_t *out = screen;
		uint8_t packets = *data++;

		for (; packets > 0; packets--) {
			int8_t count = (int8_t) *data++;

			if (count >= 0) {
				memset(out, *data++, count);
				out += count;
			} else {
				memcpy(out, data, -count);
				out += -count;
				data += -count;
			}
		}
	}
}

/* The first line changed and how many follow, then per line a count of packets, each a skip and
 * either bytes to copy or, when the count is negative, one byte repeated. */
static void DecodeDelta(const uint8_t *data, uint8_t *screen)
{
	int16_t lines;

	screen += Word(data) * LINE_BYTES;
	lines = (int16_t) Word(data + 2);
	data += 4;
	for (; lines > 0; lines--, screen += LINE_BYTES) {
		uint8_t *out = screen;
		uint8_t packets = *data++;

		for (; packets > 0; packets--) {
			int8_t count;

			out += *data++;
			count = (int8_t) *data++;
			if (count >= 0) {
				memcpy(out, data, count);
				out += count;
				data += count;
			} else {
				memset(out, *data++, -count);
				out += -count;
			}
		}
	}
}

/* A count of packets, each a number of colors to skip and a number to copy (0 for 256). */
static void DecodeColor(const uint8_t *data, uint8_t *palette)
{
	uint16_t packets = Word(data);

	data += 2;
	for (; packets > 0; packets--) {
		uint16_t count;

		palette += *data++ * 3;
		count = *data++;
		if (count == 0)
			count = 256;
		memcpy(palette, data, count * 3);
		palette += count * 3;
		data += count * 3;
	}
}

/* Finds the FLIC chunk called name in the file and loads it. */
uint8_t Flic::load(IffFile *file, const char *name)
{
	FindNamedChunk(file, "FLIC", name);
	delete[] data;
	size = file->chunk.size;
	data = new uint8_t[size];
	file->readChunk(data);
	frames = Word(data + 6);
	rewind();
	return 1;
}

/* Reads the next frame and decodes it into the bitmap. */
uint8_t Flic::nextFrame()
{
	const uint8_t *frame = data + next;
	int32_t frameSize = (int32_t) Long(frame);

	if (Word(frame + 4) != FRAME_MAGIC) {
		next += 6;
		return 0;
	}
	next += frameSize;
	decode(frame + FRAME_HEADER, Word(frame + 6));
	return 1;
}

void Flic::decode(const uint8_t *chunk, uint16_t chunks)
{
	for (uint16_t i = 1; i <= chunks; i++) {
		int32_t size = (int32_t) Long(chunk) - 6;
		uint16_t type = Word(chunk + 4);

		chunk += 6;
		switch (type) {
		case FLI_COLOR:
			DecodeColor(chunk, palette);
			chunk += size;
			break;
		case FLI_LC:
			DecodeDelta(chunk, bitmap);
			chunk += size;
			break;
		case FLI_BLACK:
			memset(bitmap, 0, SCREEN_BYTES);
			break;
		case FLI_BRUN:
			DecodeBrun(chunk, bitmap, 200);
			chunk += size;
			break;
		case FLI_COPY:
			/* DOS copied the bitmap over its own copy of the frame: the bitmap stays. */
			break;
		}
	}
}

/* Sets the frame's palette and copies its bitmap to the view. */
void Flic::show(::View *view)
{
	Palette colors;

	colors.load(0, 256, (Rgb *) palette);
	colors.write(0, 256, 0);
	memcpy(ViewPixels(view), bitmap, SCREEN_BYTES);
}

/* Copies the bitmap to the view with every color set to one, ready to fade in. */
void Flic::showInColor(::View *view, Rgb *color)
{
	Rgb colors[256];

	for (int16_t i = 0; i < 256; i++)
		colors[i] = *color;
	WriteDac(0, 256, colors);
	memcpy(ViewPixels(view), bitmap, SCREEN_BYTES);
}

}
