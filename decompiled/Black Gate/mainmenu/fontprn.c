/* Black Gate MAINMENU.EXE, resident segment 4 (file offsets 0x00b609 to 0x00b7d8, 463 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "itable.h"

void FontTextPrinter::setShape(long data, int flags)
{
	shape = data;
	drawFlags = flags;
}

void FontTextPrinter::drawChar(View *view, int px, int py, char c)
{
	DrawFrame(view, px, py, shape, c, drawFlags);
}

int FontTextPrinter::charHeight(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.y1 - bounds.y0 + leading + 1;
}

int FontTextPrinter::charWidth(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, c, drawFlags);
	return bounds.x1 - bounds.x0 + spacing + 1;
}

int FontTextPrinter::textWidth(char *text)
{
	return TextPrinter::textWidth(text);
}

/* Width of the first count characters of text. */
int FontTextPrinter::measure(char *text, int count)
{
	int width = 0;
	char c;

	while (count-- && (c = *text++) != 0) {
		width += charWidth(c);
		width += spacing;
	}
	return width;
}

/* Height of the tallest character in text, plus the leading. */
int FontTextPrinter::textHeight(char *text)
{
	int height = 0;
	char c;

	while ((c = *text++) != 0) {
		int h = charHeight(c);
		if (h > height)
			height = h;
	}
	return height + leading;
}

int FontTextPrinter::frameCount()
{
	return GetShapeFrameCount(shape, drawFlags);
}

void FontTextPrinter::printFormatted(int px, int py, char *format, ...)
{
	va_list args;

	if (format != WorkString) {
		va_start(args, format);
		vsprintf(WorkString, format, args);
	}
	print(px, py, WorkString);
}
