/* Black Gate MAINMENU.EXE, resident segment 16 (file offsets 0x00e15a to 0x00e867, 1805 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "lowlevel.h"
#include "textspr.h"

/*
 * Copies at most size characters of a label. "#S" marks the sprite, "#n" becomes
 * the control code n and "##" is a plain '#'.
 */
void CopyLabel(TextSprite *sprite, char far *dest, char far *src, int size)
{
	unsigned char escaped;
	char c;

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

TextSprite::TextSprite(char *name, char back, Screen *screen, char keep)
	: Sprite(name, back, screen, keep)
{
	init();
}

TextSprite::TextSprite(char *flexName, int entry, char back, Screen *screen, char keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	init();
}

TextSprite::TextSprite(void far *font, char back, Screen *screen, char keep)
	: Sprite(font, back, screen, keep)
{
	init();
}

TextSprite::~TextSprite()
{
	if (text)
		delete text;
}

/* Takes count characters of s, or all of it when count is negative, after any command. */
void TextSprite::setText(char far *s, int count)
{
	if (count < 0)
		count = _fstrlen(s);
	if (text && size <= count) {
		delete text;
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

		GetFrameBounds(&bounds, 0, 0, (long) image, frame, imageFlags);
		lineWidth = bounds.x1 - bounds.x0 + 1;
		imageHeight = bounds.y1 - bounds.y0 + 1;
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

void TextSprite::drawChar(View *view, int px, int py, char c)
{
	DrawFrame(view, px, py, (long) shape.data, c, drawFlags);
}

void TextSprite::reset()
{
}

int TextSprite::charWidth(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, (long) shape.data, c, drawFlags);
	return bounds.x1 - bounds.x0 + 1;
}

int TextSprite::charHeight(char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, (long) shape.data, c, drawFlags);
	return bounds.y1 - bounds.y0 + 1;
}

/* Saves the full-width band of rows behind the text. */
void TextSprite::saveUnder()
{
	if (keepUnder) {
		Rect band(0, Sprite::y - lineHeight + descent, SCREEN_WIDTH - 1, Sprite::y + descent);

		SaveRect((int) this, (long) under.data, &band, 0);
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
	if (showImage) {
		if (image)
			DrawFrame(this, left, Sprite::y, (long) image, frame, imageFlags);
	} else
		print(left, Sprite::y, text);
}

/* Puts back the band of rows saved from under the text. */
void TextSprite::restoreUnder()
{
	if (keepUnder) {
		Rect band(0, underY - lineHeight + descent, SCREEN_WIDTH - 1, underY + descent);

		RestoreRect((int) this, (long) under.data, &band, 0);
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

	GetShapeBounds(shape.data, &bounds, drawFlags);
	lineHeight = bounds.y1 - bounds.y0;
	descent = bounds.y1;
}

/* Shows frames of a shape loaded elsewhere. */
void TextSprite::setImage(void far *data)
{
	image = data;
	imageFlags = 0x111;
}

/* Shows frames of a shape the sprite loaded itself. */
void TextSprite::setOwnImage(void far *data)
{
	image = data;
	imageFlags = 0;
}

void TextSprite::setAlign(unsigned char a)
{
	align = a;
}

/* Whether the text or image lies wholly above the top of the screen. */
char TextSprite::aboveTop()
{
	if (showImage)
		return Sprite::y + imageHeight < 0;
	return Sprite::y + descent < 0;
}

/* Whether it lies wholly below the bottom. */
char TextSprite::belowBottom()
{
	if (showImage)
		return Sprite::y > SCREEN_HEIGHT - 1;
	return Sprite::y + descent - lineHeight > SCREEN_HEIGHT - 1;
}

/* Whether it lies wholly above the middle row. */
char TextSprite::aboveMiddle()
{
	if (showImage)
		return Sprite::y + imageHeight < SCREEN_HEIGHT / 2;
	return Sprite::y + descent < SCREEN_HEIGHT / 2;
}

/* Whether it lies wholly below the middle row. */
char TextSprite::belowMiddle()
{
	if (showImage)
		return Sprite::y >= SCREEN_HEIGHT / 2;
	return Sprite::y + descent - lineHeight >= SCREEN_HEIGHT / 2;
}
