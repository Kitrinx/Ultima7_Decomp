/* Black Gate U7.EXE, resident segment 58 (file offsets 0x0225d3 to 0x022730, 349 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7manage.h"
#include "itable.h"

ProportionalTextPrinter YellowTextPrinter;

/* Extra pixels between letters and between lines, per font. */
int FontLetterSpacing[8] = { -1, 1, 0, 1, 0, 0, 0, -1 };
int FontLineSpacing[8] = { -1, 0, -1, 0, 1, 1, 1, -1 };

ProportionalTextPrinter::~ProportionalTextPrinter()
{
}

void ProportionalTextPrinter::drawChar(View *surface, int px, int py, char c)
{
	FontTextPrinter::drawChar(surface, px, py, c);
}

/* Fonts 0 to 7 are the first eight font shapes; any other number picks font 0. */
void ProportionalTextPrinter::setFont(unsigned char n)
{
	unsigned char index;

	if (n < 8)
		index = n;
	else
		index = 0;
	font = index + FIRST_FONT_SHAPE;
	spacing = FontLetterSpacing[index];
	leading = FontLineSpacing[index];
}

int ProportionalTextPrinter::charWidth(char c)
{
	int width;
	int height;

	gShapeManager.getFrameSize(&width, &height, font, c);
	return width + spacing;
}

int ProportionalTextPrinter::charHeight(char)
{
	int width;
	int height;

	gShapeManager.getShapeSize(&width, &height, font);
	return height + leading;
}

int ProportionalTextPrinter::textWidth(char *s)
{
	int width = 0;

	for (; *s; s++) {
		width += charWidth(*s);
		width += spacing;
	}
	return width;
}
