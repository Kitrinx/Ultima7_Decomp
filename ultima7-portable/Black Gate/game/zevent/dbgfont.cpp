/* Black Gate U7.EXE, resident segment 15 (file offsets 0x01205a to 0x0121f4, 410 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <stdlib.h>
#include <string.h>
#include "dbgfont.h"

#define GLYPH_HEIGHT 14     /* the 8x14 ROM font */

uint8_t *RomFontGlyphs = 0;
char *DebugAreaValue = 0;
char *DebugAreaLine = 0;
uint8_t RomTextColor = 15;
uint8_t RomTextBackground = 0;
uint8_t *RomTextScreen = 0;
int16_t RomTextPitch = 320;
void (*DebugHook)(void) = 0;
static const char DebugAreaBufferStart[80] = "Area=";
char DebugAreaBuffer[80];

RomFontLoader::RomFontLoader()
{
	RomFontGlyphs = 0;      /* no ROM font on the host, so debug text draws nothing */
	DebugAreaValue = DebugAreaBuffer + strlen(DebugAreaBuffer);
	DebugAreaLine = DebugAreaBuffer;
}

void SetDebugArea(int32_t value)
{
	ltoa(value, DebugAreaValue, 16);
}

char *GetDebugAreaText(void)
{
	return DebugAreaValue;
}

void DrawRomChar(int8_t ch, int16_t x, int16_t y)
{
	if (RomFontGlyphs == 0 || RomTextScreen == 0)
		return;
	int16_t rows = GLYPH_HEIGHT;
	uint8_t *glyph = RomFontGlyphs + ch * GLYPH_HEIGHT;
	uint8_t *dst = RomTextScreen + x + y * RomTextPitch;

	while (rows--) {
		uint8_t bits = *glyph++;
		uint8_t mask;

		for (mask = 0x80; mask != 0; mask >>= 1, dst++) {
			if (bits & mask)
				*dst = RomTextColor;
			else
				*dst = RomTextBackground;
		}
		dst += RomTextPitch - 8;
	}
}

extern "C" void DrawRomText(char *s, int16_t x, int16_t row)
{
	int16_t y = row * GLYPH_HEIGHT;

	while (*s) {
		DrawRomChar(*s++, x, y);
		x += 8;
	}
}

void DrawDebugArea(void)
{
	int16_t x = 0;
	int16_t y = 185;
	char *s = DebugAreaLine;

	while (*s) {
		DrawRomChar(*s++, x, y);
		x += 8;
	}
}

void SetDebugHook(void (*hook)(void))
{
	DebugHook = hook;
}

void CallDebugHook(void)
{
	if (DebugHook)
		(*DebugHook)();
}

extern "C" void ResetDbgfontGlobals(void)
{
	RomFontGlyphs = 0;
	DebugAreaValue = 0;
	DebugAreaLine = 0;
	RomTextColor = 15;
	RomTextBackground = 0;
	RomTextScreen = 0;
	RomTextPitch = 320;
	DebugHook = 0;
	memcpy(DebugAreaBuffer, DebugAreaBufferStart, sizeof(DebugAreaBuffer));
}
