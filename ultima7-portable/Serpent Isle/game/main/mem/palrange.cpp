/* Loads ranges of palette colors, each picked through a table of indices. */

#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "lowlevel.h"

/* What the game last loaded, for reading back. */
static uint8_t PaletteShadow[256 * 3];

/* Colors first to first + num - 1 from 6-byte entries: three 8.8 words, the high bytes 6-bit. */
extern "C" void SetPaletteRange(int32_t table, int16_t *indices, int16_t first, int16_t num)
{
	uint8_t *rgb;
	uint8_t *entry;
	int16_t i;

	for (i = 0; i < num; i++) {
		entry = LINEAR(table + (uint16_t) indices[first + i] * 6);
		rgb = &PaletteShadow[(uint8_t) (first + i) * 3];
		rgb[0] = entry[1] & 0x3f;
		rgb[1] = entry[3] & 0x3f;
		rgb[2] = entry[5] & 0x3f;
		plat_video_set_palette((uint8_t) (first + i), 1, rgb);
	}
}

/* The same from 3-byte entries, held anywhere. */
extern "C" void SetPaletteRangeBytes(const uint8_t *table, int16_t *indices, int16_t first, int16_t num)
{
	uint8_t *rgb;
	const uint8_t *entry;
	int16_t i;

	for (i = 0; i < num; i++) {
		entry = table + (uint16_t) indices[first + i] * 3;
		rgb = &PaletteShadow[(uint8_t) (first + i) * 3];
		rgb[0] = entry[0] & 0x3f;
		rgb[1] = entry[1] & 0x3f;
		rgb[2] = entry[2] & 0x3f;
		plat_video_set_palette((uint8_t) (first + i), 1, rgb);
	}
}

/* Reads colors back into 3-byte entries. */
extern "C" void ReadPaletteRange(int32_t table, int16_t *indices, int16_t first, int16_t num)
{
	int16_t i;

	for (i = 0; i < num; i++)
		memcpy(LINEAR(table + (uint16_t) indices[first + i] * 3), &PaletteShadow[(uint8_t) (first + i) * 3], 3);
}

extern "C" void ResetPalrangeGlobals(void)
{
	memset(PaletteShadow, 0, sizeof(PaletteShadow));
}
