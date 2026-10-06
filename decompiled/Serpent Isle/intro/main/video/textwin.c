/* Serpent Isle INTRO.EXE, resident segment 14 (file offsets 0x00bc25 to 0x00c498, 2163 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <mem.h>
#include <stdlib.h>
#include <string.h>
#include "font.h"
#include "textwin.h"

#define TEXT_SIZE       200

/* Draws on the pixels of where, through its own copy of the view. */
TextWindow::TextWindow(View *where, Font *f)
{
	view = *where;
	setFont(f);
}

unsigned char TextWindow::setForeground(unsigned char color)
{
	font->setColor(color);
	return foreground = color;
}

unsigned char TextWindow::setBackground(unsigned char color)
{
	font->setBackground(color);
	return background = color;
}

void TextWindow::setClip(int x0, int y0, int x1, int y1)
{
	view.clip.set(x0, y0, x1, y1);
	home();
}

void TextWindow::clear(unsigned char color)
{
	if (color == BACKGROUND)
		color = background;
	view.clear(color);
}

/* Clears the whole window and puts the cursor at its corner. */
void pascal TextWindow::erase(unsigned char color)
{
	if (hasFont()) {
		if (color == BACKGROUND)
			color = background;
		view.clear(color);
		home();
	}
}

/* Clears from the cursor to the end of its line. */
void pascal TextWindow::eraseLine(unsigned char color)
{
	int y, last;

	if (hasFont()) {
		if (color == BACKGROUND)
			color = background;
		y = row();
		last = y + font->charHeight() + font->leading - 1;
		if (bottom() < y)
			return;
		view.fill(column(), y, right(), last, color);
	}
}

/* Clears the rest of the cursor's line and every line below it. */
void pascal TextWindow::eraseBelow(unsigned char color)
{
	int y;

	if (hasFont()) {
		if (color == BACKGROUND)
			color = background;
		eraseLine(color);
		y = row();
		y += font->charHeight() + font->leading;
		view.fill(left(), y, right(), bottom(), color);
	}
}

void TextWindow::setHorizontal(int x0, int x1)
{
	view.clip.x0 = x0;
	view.clip.x1 = x1;
	home();
}

void TextWindow::setVertical(int y0, int y1)
{
	view.clip.y0 = y0;
	view.clip.y1 = y1;
	home();
}

/* Draws a printable character at the cursor and moves past it. */
void TextWindow::putChar(char c)
{
	if (c > 31) {
		font->drawChar(&view, c, cursor.x0, cursor.y0);
		setX(column() + font->charWidth(c) + font->spacing);
	}
}

void TextWindow::putLines(char *text)
{
	int c;
	int lineHeight;

	lineHeight = font->charHeight() + font->leading + font->spacing - 1;
	applyJustify(text);
	while ((c = *text++) != 0L) {
		if (c == '\r' || c == '\n') {
			setY(row() + lineHeight);
			applyJustify(text);
		} else
			putChar(c);
	}
}

/*
 * printf-style codes, plus: %X and %Y move the cursor, %F and %B set the
 * colours, %J the justification, %P pauses. %U and %D take longs.
 */
void TextWindow::format(char *fmt, va_list args)
{
	int c;
	int code;
	char text[TEXT_SIZE];
	char number[TEXT_SIZE];

	memset(text, 0, TEXT_SIZE);
	memset(number, 0, TEXT_SIZE);
	while ((c = *fmt++) != 0L) {
		switch (c) {
		case '%':
			code = *fmt++;
			switch (code) {
			case 'd':
				strcat(text, itoa(va_arg(args, int), number, 10));
				break;
			case 'u':
				strcat(text, ultoa(va_arg(args, unsigned), number, 10));
				break;
			case 'D':
				strcat(text, ltoa(va_arg(args, long), number, 10));
				break;
			case 'U':
				strcat(text, ultoa(va_arg(args, unsigned long), number, 10));
				break;
			case 'x':
				strcat(text, strupr(ultoa(va_arg(args, unsigned), number, 16)));
				break;
			case 'c':
				number[0] = va_arg(args, int);
				number[1] = 0;
				strcat(text, number);
				break;
			case 'S':
				strcat(text, va_arg(args, char *));
				break;
			case 'X':
				setX(va_arg(args, int));
				break;
			case 'Y':
				setY(va_arg(args, int));
				break;
			case 'B':
				background = va_arg(args, int);
				break;
			case 'F':
				foreground = va_arg(args, int);
				break;
			case 'P':
				pause();
				break;
			case 'J':
				justify = va_arg(args, int);
				break;
			default:
				number[0] = c;
				number[1] = 0;
				strcat(text, number);
				break;
			}
			break;
		default:
			number[0] = c;
			number[1] = 0;
			strcat(text, number);
			break;
		}
	}
	putLines(text);
}

void TextWindow::print(char *fmt, ...)
{
	va_list args;

	if (font) {
		va_start(args, fmt);
		format(fmt, args);
	}
}

void TextWindow::pause()
{
}

/* The width of the text up to the end of its line. */
int TextWindow::measureLine(char *text)
{
	int width = 0;
	int count = 0;

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
		setX(left());
		break;
	case JUSTIFY_RIGHT:
		setX(right() - measureLine(text));
		break;
	case JUSTIFY_CENTER:
		setX((right() - measureLine(text)) / 2);
		break;
	case JUSTIFY_FULL:
		setX(left());
		break;
	}
}
