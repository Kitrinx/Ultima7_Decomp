/* Black Gate INTRO.EXE, resident segment 7 (file offsets 0x00aa11 to 0x00ad2e, 797 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "textspr.h"

TextSprite::~TextSprite()
{
	if (text)
		delete text;
}

void TextSprite::setText(char far *s)
{
	int count = _fstrlen(s);

	if (text && size <= count) {
		delete text;
		text = 0;
	}
	if (!text) {
		text = new char[count + 1];
		size = count + 1;
	}
	_fstrncpy(text, s, count + 1);
	lineWidth = TextPrinter::textWidth(text);
}

/* Measures the font, and makes room under the sprite for a full-width band of rows as tall as it. */
void TextSprite::measure()
{
	Rect bounds;

	GetShapeBounds(shape.data, &bounds);
	lineHeight = bounds.y1 - bounds.y0;
	descent = bounds.y1;
	under.release();
	under.allocate((lineHeight + 1) * SCREEN_WIDTH);
}

void TextSprite::drawChar(View *view, int px, int py, char c)
{
	DrawScaledFrame(view, px, py, shape.data, c, angle, scale, 0);
}

void TextSprite::reset()
{
}

int TextSprite::charWidth(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.data, c);
	return bounds.x1 - bounds.x0 + 1;
}

int TextSprite::charHeight(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.data, c);
	return bounds.y1 - bounds.y0 + 1;
}

/* Saves the full-width band of rows behind the text. */
void TextSprite::saveUnder()
{
	if (keepUnder) {
		Rect band(0, Sprite::y - lineHeight + descent, SCREEN_WIDTH - 1, Sprite::y + descent);

		SaveRect(this, under.data, &band);
		underX = 0;
		underY = Sprite::y;
		drawn = 1;
	}
}

void TextSprite::draw()
{
	int left = Sprite::x;

	switch (align) {
	case ALIGN_LEFT:
		left = Sprite::x;
		break;
	case ALIGN_CENTRE:
		left = Sprite::x - (lineWidth >> 1);
		break;
	case ALIGN_RIGHT:
		left = Sprite::x - lineWidth;
		break;
	}
	target = this;
	print(left, Sprite::y, text);
}

/* Puts back the band of rows saved from under the text. */
void TextSprite::restoreUnder()
{
	if (keepUnder) {
		Rect band(0, underY - lineHeight + descent, SCREEN_WIDTH - 1, underY + descent);

		RestoreRect(this, under.data, &band);
	}
}

void TextSprite::init()
{
	text = 0;
	size = 0;
	align = ALIGN_LEFT;
	lineWidth = 0;
	lineHeight = 0;
	descent = 0;
}
