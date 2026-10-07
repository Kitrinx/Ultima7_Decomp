/* Serpent Isle MAINMENU.EXE, resident segment 4 (file offsets 0x00bb43 to 0x00bdba, 631 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdarg.h>
#include <stdio.h>
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "itable.h"
#include "fileutil.h"
#include "../main/mem/errors.h"

namespace Shared {

void FontTextPrinter::setShape(int32_t data, int16_t flags)
{
	shape = data;
	drawFlags = flags;
}

void FontTextPrinter::drawChar(View *view, int16_t px, int16_t py, int8_t c)
{
	DrawFrame(view, px, py, shape, c, drawFlags);
}

int16_t FontTextPrinter::charHeight(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.bottom() - bounds.top() + 1 + leading;
}

int16_t FontTextPrinter::charWidth(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.right() - bounds.left() + 1 + spacing;
}

int16_t FontTextPrinter::textWidth(char *text)
{
	return TextPrinter::textWidth(text);
}

/* Width of the first count characters of text. */
int16_t FontTextPrinter::measure(char *text, int16_t count)
{
	int16_t width = 0;
	int8_t c;

	while (count-- && (c = *text++) != 0) {
		width += charWidth(c);
		width += spacing;
	}
	return width;
}

/* Height of the tallest character in text, plus the leading. */
int16_t FontTextPrinter::textHeight(char *text)
{
	int16_t height = 0;
	int8_t c;

	while ((c = *text++) != 0) {
		int16_t h = charHeight(c);
		if (h > height)
			height = h;
	}
	return height + leading;
}

int16_t FontTextPrinter::frameCount()
{
	return GetShapeFrameCount(shape, drawFlags);
}

void FontTextPrinter::printFormatted(int16_t px, int16_t py, char *format, ...)
{
	va_list args;

	if (format != GetWorkString(&WorkString)) {
		va_start(args, format);
		vsnprintf(GetWorkString(&WorkString), WorkstringSize, format, args);
	}
	print(px, py, GetWorkString(&WorkString));
}

}
