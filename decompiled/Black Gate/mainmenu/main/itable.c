/* Black Gate MAINMENU.EXE, resident segment 39 (file offsets 0x012224 to 0x012338, 276 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "itable.h"

TextPrinter::~TextPrinter()
{
}

TextPrinter::TextPrinter()
{
	target = &ScreenView;
	x = 0;
	y = 0;
	spacing = 0;
	leading = 0;
}

void TextPrinter::printChar(char c)
{
	drawChar(target, x, y, c);
	if (c == '\n') {
		y += charHeight(c) + leading;
		x = target->clip.x0;
	} else if (c == '\r') {
		x = target->clip.x0;
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
