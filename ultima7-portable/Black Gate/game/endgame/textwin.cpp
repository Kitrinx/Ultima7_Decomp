/* Black Gate ENDGAME.EXE: shapefnt.c and textwin.c. */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "endview.h"
#include "textwin.h"

namespace Endgame {

#define TEXT_SIZE       200

/* Takes the shapes and sets the spacing from the capital A. */
ShapeFont::ShapeFont(int32_t shapes)
{
	shape = shapes;
	baseline = charHeight();
	setSpacing(-1, 2, 0);
}

void ShapeFont::setSpacing(int16_t space, int16_t gap, int16_t lines)
{
	spacing = gap;
	leading = lines;
	if (space == -1)
		spaceWidth = charWidth('-');
	else
		spaceWidth = space;
}

/* The height of a capital, from the top of 'A' to the baseline. */
int16_t ShapeFont::charHeight()
{
	::Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape, 'A', 0);
	return (int16_t) (abs(bounds.y0) + 1);
}

void ShapeFont::drawChar(::View *view, uint8_t c, int16_t x, int16_t y)
{
	DrawShape(view, x, y + baseline, shape, c);
}

int16_t ShapeFont::charWidth(uint8_t c)
{
	::Rect bounds;

	if (c == ' ')
		return spaceWidth;
	GetFrameBounds(&bounds, 0, 0, shape, c, 0);
	return bounds.x1 + 1;
}

/* Draws on the pixels of where, through its own copy of the view. */
TextWindow::TextWindow(::View *where, ShapeFont *f)
{
	view = *where;
	font = f;
	cursorX = cursorY = 0;
	justify = foreground = background = 0;
}

/* Draws a printable character at the cursor and moves past it. */
void TextWindow::putChar(char c)
{
	if (c > 31) {
		font->drawChar(&view, c, cursorX, cursorY);
		setX(column() + font->charWidth(c) + font->spacing);
	}
}

void TextWindow::putLines(char *text)
{
	int16_t c;
	int16_t lineHeight = font->charHeight() + font->leading + font->spacing - 1;

	applyJustify(text);
	while ((c = *text++) != 0) {
		if (c == '\r' || c == '\n') {
			setY(row() + lineHeight);
			applyJustify(text);
		} else
			putChar((char) c);
	}
}

/* printf-style codes, plus: %X and %Y move the cursor, %F and %B set the colours, %J the
 * justification. %U and %D take longs. */
void TextWindow::format(const char *fmt, va_list args)
{
	int16_t c;
	char text[TEXT_SIZE];
	char number[TEXT_SIZE];

	memset(text, 0, TEXT_SIZE);
	memset(number, 0, TEXT_SIZE);
	while ((c = *fmt++) != 0) {
		if (c != '%') {
			number[0] = (char) c;
			number[1] = 0;
			strcat(text, number);
			continue;
		}
		switch (*fmt++) {
		case 'd':
			snprintf(number, TEXT_SIZE, "%d", va_arg(args, int));
			strcat(text, number);
			break;
		case 'u':
			snprintf(number, TEXT_SIZE, "%u", va_arg(args, unsigned));
			strcat(text, number);
			break;
		case 'D':
			snprintf(number, TEXT_SIZE, "%ld", (long) va_arg(args, int32_t));
			strcat(text, number);
			break;
		case 'U':
			snprintf(number, TEXT_SIZE, "%lu", (unsigned long) va_arg(args, uint32_t));
			strcat(text, number);
			break;
		case 'x':
			snprintf(number, TEXT_SIZE, "%X", va_arg(args, unsigned));
			strcat(text, number);
			break;
		case 'c':
			number[0] = (char) va_arg(args, int);
			number[1] = 0;
			strcat(text, number);
			break;
		case 'S':
			strcat(text, va_arg(args, char *));
			break;
		case 'X':
			setX((int16_t) va_arg(args, int));
			break;
		case 'Y':
			setY((int16_t) va_arg(args, int));
			break;
		case 'B':
			background = (uint8_t) va_arg(args, int);
			break;
		case 'F':
			foreground = (uint8_t) va_arg(args, int);
			break;
		case 'P':
			break;
		case 'J':
			justify = (uint8_t) va_arg(args, int);
			break;
		default:
			/* an unknown code leaves the percent sign */
			number[0] = '%';
			number[1] = 0;
			strcat(text, number);
			break;
		}
	}
	putLines(text);
}

void TextWindow::print(const char *fmt, ...)
{
	va_list args;

	if (font) {
		va_start(args, fmt);
		format(fmt, args);
		va_end(args);
	}
}

/* The width of the text up to the end of its line. */
int16_t TextWindow::measureLine(char *text)
{
	int16_t width = 0;
	int16_t count = 0;

	while (*text != '\n' && *text != 0) {
		width += font->charWidth(*text);
		text++;
		count++;
	}
	if (count)
		width += font->spacing * (count - 1);
	return width;
}

/* Puts the cursor where the next line of text starts. */
void TextWindow::applyJustify(char *text)
{
	switch (justify) {
	case JUSTIFY_LEFT:
	case JUSTIFY_FULL:
		setX(left());
		break;
	case JUSTIFY_RIGHT:
		setX(right() - measureLine(text));
		break;
	case JUSTIFY_CENTER:
		setX((right() - measureLine(text)) / 2);
		break;
	}
}

}
