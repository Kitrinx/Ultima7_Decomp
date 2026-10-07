/* Serpent Isle SI.EXE, resident segment 34 (file offsets 0x019aa2 to 0x019bff, 349 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Filename and folder inferred.
 */

#include "u7port.h"
#include <new>
#include "u7manage.h"
#include "itable.h"

ProportionalTextPrinter YellowTextPrinter;

/* Extra pixels between letters and between lines, per font. */
const int16_t FontLetterSpacing[11] = { -1, 1, 0, 1, 0, 0, 0, -1, 0, 0, 0 };
const int16_t FontLineSpacing[11] = { -1, 0, -1, 0, 1, 1, 1, -1, 1, 1, 1 };

ProportionalTextPrinter::~ProportionalTextPrinter()
{
}

void ProportionalTextPrinter::drawChar(View *surface, int16_t px, int16_t py, int8_t c)
{
	FontTextPrinter::drawChar(surface, px, py, c);
}

/* Fonts 0 to 10 use consecutive font shapes; any other number picks font 0. */
void ProportionalTextPrinter::setFont(uint8_t n)
{
	uint8_t index;

	if (n < 11)
		index = n;
	else
		index = 0;
	font = index + FIRST_FONT_SHAPE;
	spacing = FontLetterSpacing[index];
	leading = FontLineSpacing[index];
}

int16_t ProportionalTextPrinter::charWidth(int8_t c)
{
	int16_t width;
	int16_t height;

	gShapeManager.getFrameSize(&width, &height, font, c);
	return width + spacing;
}

int16_t ProportionalTextPrinter::charHeight(int8_t)
{
	int16_t width;
	int16_t height;

	gShapeManager.getShapeSize(&width, &height, font);
	return height + leading;
}

int16_t ProportionalTextPrinter::textWidth(char *s)
{
	int16_t width = 0;

	for (; *s; s++) {
		width += charWidth(*s);
		width += spacing;
	}
	return width;
}

extern "C" void ResetU7fontGlobals(void)
{
	memset((void *)&YellowTextPrinter, 0, sizeof(YellowTextPrinter));
}

extern "C" void ConstructU7fontGlobals(void)
{
	new (&YellowTextPrinter) ProportionalTextPrinter();
}
