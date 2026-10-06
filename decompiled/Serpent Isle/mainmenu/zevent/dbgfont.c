/* Serpent Isle MAINMENU.EXE, one module of resident segment 31 (file offsets 0x011a13 to 0x011b9b, 392 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include <string.h>
#include "dbgfont.h"

#define GLYPH_HEIGHT 14     /* the 8x14 ROM font */

unsigned char far *RomFontGlyphs = 0;
char *DebugAreaValue = 0;
char far *DebugAreaLine = 0;
unsigned char RomTextColor = 15;
unsigned char RomTextBackground = 0;
unsigned char far *RomTextScreen = (unsigned char far *) 0xa0000000L;
int RomTextPitch = 320;
void (*DebugHook)(void) = 0;
char DebugAreaBuffer[80] = "Area=";

RomFontLoader::RomFontLoader()
{
	RomFontGlyphs = GetRomFont(2);
	DebugAreaValue = DebugAreaBuffer + strlen(DebugAreaBuffer);
	DebugAreaLine = DebugAreaBuffer;
}

void SetDebugArea(long value)
{
	ltoa(value, DebugAreaValue, 16);
}

char *GetDebugAreaText(void)
{
	return DebugAreaValue;
}

void DrawRomChar(char ch, int x, int y)
{
	int rows = GLYPH_HEIGHT;
	unsigned char far *glyph = RomFontGlyphs + ch * GLYPH_HEIGHT;
	unsigned char far *dst = RomTextScreen + x + y * RomTextPitch;

	while (rows--) {
		unsigned char bits = *glyph++;
		unsigned char mask;

		for (mask = 0x80; mask != 0; mask >>= 1, dst++) {
			if (bits & mask)
				*dst = RomTextColor;
			else
				*dst = RomTextBackground;
		}
		dst += RomTextPitch - 8;
	}
}

extern "C" void DrawRomText(char far *s, int x, int row)
{
	int y = row * GLYPH_HEIGHT;

	while (*s) {
		DrawRomChar(*s++, x, y);
		x += 8;
	}
}

void DrawDebugArea(void)
{
	int x = 0;
	int y = 185;
	char far *s = DebugAreaLine;

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
