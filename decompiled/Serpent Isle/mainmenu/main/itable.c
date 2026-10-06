/* Serpent Isle MAINMENU.EXE, resident segment 38 (file offsets 0x01260c to 0x01274b, 319 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "itable.h"

TextPrinter::~TextPrinter()
{
}

TextPrinter::TextPrinter()
{
	setTarget(&ScreenView);
	move(0, 0);
	setSpacing(0);
	setLeading(0);
}

void TextPrinter::printChar(char c)
{
	drawChar(target, x, y, c);
	if (c == '\n') {
		y += charHeight(c) + leading;
		x = target->clip.getLeft();
	} else if (c == '\r') {
		x = target->clip.getLeft();
	} else {
		x += charWidth(c) + spacing;
	}
}

void TextPrinter::printString(char *text)
{
	for (; *text; text++)
		printChar(*text);
}

int TextPrinter::textWidth(char *text)
{
	int width = 0;

	for (; *text; text++) {
		width += charWidth(*text);
		width += spacing;
	}
	return width;
}
