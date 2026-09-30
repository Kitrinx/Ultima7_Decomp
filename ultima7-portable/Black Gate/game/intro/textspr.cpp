/* Black Gate INTRO.EXE, textspr.c and shapebnd.c: the subtitle sprite. */

#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "textspr.h"
#include "scalfram.h"

namespace Intro {

TextSprite::~TextSprite()
{
	if (text)
		delete[] text;
}

void TextSprite::setText(char *s)
{
	int16_t count = (int16_t) strlen(s);

	if (text && size <= count) {
		delete[] text;
		text = 0;
	}
	if (!text) {
		text = new char[count + 1];
		size = count + 1;
	}
	strncpy(text, s, count + 1);
	lineWidth = TextPrinter::textWidth(text);
}

/* Measures the font, and makes room under the sprite for a full-width band of rows as tall as it. */
void TextSprite::measure()
{
	Rect bounds;

	GetShapeBounds(shape.linear(), &bounds);
	lineHeight = bounds.y1 - bounds.y0;
	descent = bounds.y1;
	under.release();
	under.allocate((int32_t) (lineHeight + 1) * SCREEN_WIDTH);
}

void TextSprite::drawChar(View *view, int16_t px, int16_t py, int8_t c)
{
	DrawScaledFrame(view, px, py, shape.linear(), c, angle, scale, 0);
}

void TextSprite::reset()
{
}

int16_t TextSprite::charWidth(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.linear(), c, 0);
	return bounds.x1 - bounds.x0 + 1;
}

int16_t TextSprite::charHeight(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.linear(), c, 0);
	return bounds.y1 - bounds.y0 + 1;
}

/* Saves the full-width band of rows behind the text. */
void TextSprite::saveUnder()
{
	if (keepUnder) {
		Rect band = { 0, (int16_t) (Sprite::y - lineHeight + descent), SCREEN_WIDTH - 1,
			(int16_t) (Sprite::y + descent) };

		SaveRect(this, under.linear(), &band, 0);
		underX = 0;
		underY = Sprite::y;
		drawn = 1;
	}
}

void TextSprite::draw()
{
	int16_t left = Sprite::x;

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
		Rect band = { 0, (int16_t) (underY - lineHeight + descent), SCREEN_WIDTH - 1,
			(int16_t) (underY + descent) };

		RestoreRect(this, under.linear(), &band, 0);
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

/* The union of the bounds of every frame of a shape. */
void GetShapeBounds(int32_t shape, Rect *bounds)
{
	int16_t count;
	Rect frame;
	int16_t i;

	bounds->x0 = 0;
	bounds->y0 = 0;
	bounds->x1 = 0;
	bounds->y1 = 0;
	count = GetShapeFrameCount(shape, 0);
	for (i = 0; i < count; i++) {
		GetFrameBounds(&frame, 0, 0, shape, i, 0);
		if (frame.y0 < bounds->y0)
			bounds->y0 = frame.y0;
		if (frame.y1 > bounds->y1)
			bounds->y1 = frame.y1;
		if (frame.x0 < bounds->x0)
			bounds->x0 = frame.x0;
		if (frame.x1 > bounds->x1)
			bounds->x1 = frame.x1;
	}
}

}
