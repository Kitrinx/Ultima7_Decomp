/* Serpent Isle MAINMENU.EXE, resident segment 16 (file offsets 0x00e98d to 0x00f0ec, 1887 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 * Member-call spellings and no-inline setting inferred from emitted calls.
 */

#include "u7port.h"
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "textspr.h"

namespace MainMenu {

/*
 * Copies at most size characters of a label. "#S" marks the sprite, "#n" becomes
 * the control code n and "##" is a plain '#'.
 */
void CopyLabel(TextSprite *sprite, char *dest, char *src, int16_t size)
{
	uint8_t escaped;
	int8_t c;

	escaped = 0;
	while ((c = *src++) != 0 && size-- != 0) {
		if (escaped) {
			if (c != '#') {
				if (c == 'S' || c == 's') {
					sprite->marked = 1;
					escaped = 0;
					continue;
				}
				c -= '0';
			}
			escaped = 0;
		} else if (c == '#') {
			escaped = 1;
			continue;
		}
		*dest++ = c;
	}
	*dest = 0;
}

TextSprite::TextSprite(char *name, int8_t back, Screen *screen, int8_t keep)
	: Sprite(name, back, screen, keep)
{
	init();
}

TextSprite::TextSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	init();
}

TextSprite::TextSprite(void *font, int8_t back, Screen *screen, int8_t keep)
	: Sprite(font, back, screen, keep)
{
	init();
}

TextSprite::~TextSprite()
{
	if (text)
		delete[] text;
}

/* Takes count characters of s, or all of it when count is negative, after any command. */
void TextSprite::setText(char *s, int16_t count)
{
	if (count < 0)
		count = _fstrlen(s);
	if (text && size <= count) {
		delete[] text;
		text = 0;
	}
	if (!text) {
		text = new char[count + 1];
		size = count + 1;
	}
	showImage = 0;
	if (*s == '\\') {
		switch (s[1]) {
		case 'C':
		case 'c':
			setAlign(ALIGN_CENTRE);
			break;
		case 'L':
		case 'l':
			setAlign(ALIGN_LEFT);
			break;
		case 'R':
		case 'r':
			setAlign(ALIGN_RIGHT);
			break;
		case 'P':
		case 'p':
			setFrame(s[2] - '0');
			showImage = 1;
			s++;
			count--;
			setAlign(ALIGN_CENTRE);
			break;
		}
		s += 2;
		count -= 2;
	}
	CopyLabel(this, text, s, count);
	text[count] = 0;
	if (showImage) {
		Rect bounds;

		GetFrameBounds(&bounds, 0, 0, PointerToLinear(image), frame, imageFlags);
		lineWidth = bounds.right() - bounds.left() + 1;
		imageHeight = bounds.bottom() - bounds.top() + 1;
	} else {
		lineWidth = TextPrinter::textWidth(text);
		imageHeight = lineHeight;
	}
}

/* Makes room under the sprite for a full-width band of rows as tall as the font. */
void TextSprite::allocateUnder()
{
	under.release();
	under.allocate((lineHeight + 1) * SCREEN_WIDTH);
}

void TextSprite::drawChar(View *view, int16_t px, int16_t py, int8_t c)
{
	DrawFrame(view, px, py, shape.linear(), c, drawFlags);
}

void TextSprite::reset()
{
}

int16_t TextSprite::charWidth(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.linear(), c, drawFlags);
	return bounds.right() - bounds.left() + 1;
}

int16_t TextSprite::charHeight(int8_t c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.linear(), c, drawFlags);
	return bounds.bottom() - bounds.top() + 1;
}

/* Saves the full-width band of rows behind the text. */
void TextSprite::saveUnder()
{
	if (keepUnder) {
		Rect band;

		band.set(0, Sprite::y - lineHeight + descent, SCREEN_WIDTH - 1, Sprite::y + descent);
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
	setTarget(this);
	if (showImage) {
		if (image)
			DrawFrame((View *) this, left, Sprite::y, PointerToLinear(image), frame, imageFlags);
	} else
		print(left, Sprite::y, text);
}

/* Puts back the band of rows saved from under the text. */
void TextSprite::restoreUnder()
{
	if (keepUnder) {
		Rect band;

		band.set(0, underY - lineHeight + descent, SCREEN_WIDTH - 1, underY + descent);
		RestoreRect(this, under.linear(), &band, 0);
	}
}

/* Clears the text and image, and measures the font. */
void TextSprite::init()
{
	text = 0;
	size = 0;
	align = ALIGN_LEFT;
	lineWidth = 0;
	lineHeight = 0;
	descent = 0;
	image = 0;
	showImage = 0;
	imageHeight = 0;
	imageFlags = 0;
	marked = 0;

	Rect bounds;

	GetShapeBounds(shape.get(), &bounds, drawFlags);
	lineHeight = bounds.y1 - bounds.y;
	descent = bounds.y1;
}

/* Shows frames of a shape loaded elsewhere. */
void TextSprite::setImage(void *data)
{
	image = data;
	imageFlags = 0x111;
}

/* Shows frames of a shape the sprite loaded itself. */
void TextSprite::setOwnImage(void *data)
{
	image = data;
	imageFlags = 0;
}

void TextSprite::setAlign(uint8_t a)
{
	align = a;
}

/* Whether the text or image lies wholly above the top of the screen. */
int8_t TextSprite::aboveTop()
{
	if (showImage)
		return Sprite::y + imageHeight < 0;
	return Sprite::y + descent < 0;
}

/* Whether it lies wholly below the bottom. */
int8_t TextSprite::belowBottom()
{
	if (showImage)
		return Sprite::y > SCREEN_HEIGHT - 1;
	return Sprite::y + descent - lineHeight > SCREEN_HEIGHT - 1;
}

/* Whether it lies wholly above the middle row. */
int8_t TextSprite::aboveMiddle()
{
	if (showImage)
		return Sprite::y + imageHeight < SCREEN_HEIGHT / 2;
	return Sprite::y + descent < SCREEN_HEIGHT / 2;
}

/* Whether it lies wholly below the middle row. */
int8_t TextSprite::belowMiddle()
{
	if (showImage)
		return Sprite::y >= SCREEN_HEIGHT / 2;
	return Sprite::y + descent - lineHeight >= SCREEN_HEIGHT / 2;
}

}
