/* Black Gate MAINMENU.EXE, resident segment 18 (file offsets 0x00e918 to 0x00ecda, 962 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "oops.h"
#include "textspr.h"
#include "scroll.h"

#define LINE_BREAK  '|'
#define WHOLE_TEXT  -1      /* setText measures the string itself */

ScrollLine::ScrollLine(void far *font, char back, Screen *screen, char keep)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++) {
		if ((*line = new TextSprite(font, back, screen, keep)) == 0)
			ReportOutOfVoodooMemory();
	}
}

ScrollLine::~ScrollLine()
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		if (*line)
			delete *line;
}

void ScrollLine::moveTo(int x, int y)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->moveTo(x, y);
}

void ScrollLine::moveTo(int x, int y, int n)
{
	if (n < 2)
		lines[n]->moveTo(x, y);
}

void ScrollLine::moveBy(int dx, int dy)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->moveBy(dx, dy);
}

void ScrollLine::moveBy(int dx, int dy, int n)
{
	if (n < 2)
		lines[n]->moveBy(dx, dy);
}

/* True when both lines have scrolled off the top. */
unsigned char ScrollLine::aboveTop()
{
	char above = 1;
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		above = above && (*line)->aboveTop();
	return above;
}

unsigned char ScrollLine::aboveMiddle()
{
	char above = 1;
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		above = above && (*line)->aboveMiddle();
	return above;
}

char ScrollLine::aboveTop(int n)
{
	if (n < 2)
		return lines[n]->aboveTop();
}

char ScrollLine::aboveMiddle(int n)
{
	if (n < 2)
		return lines[n]->aboveMiddle();
}

void ScrollLine::setText(char far *text)
{
	char far *p = text;
	int length = 0;

	while (*p != 0) {
		if (*p == LINE_BREAK)
			break;
		length++;
		p++;
	}
	if (*p == LINE_BREAK) {
		lines[1]->show();
		lines[1]->setText(++p, WHOLE_TEXT);
		lines[0]->setText(text, length);
	} else {
		lines[1]->hide();
		lines[0]->setText(text, WHOLE_TEXT);
	}
}

void ScrollLine::setText(char far *text, int n)
{
	if (n < 2) {
		lines[n]->show();
		lines[n]->setText(text, WHOLE_TEXT);
	}
}

void ScrollLine::setAlign(char a)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->setAlign(a);
}

void ScrollLine::setSpacing(int spacing)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->spacing = spacing;
}

void ScrollLine::setImage(void far *data)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->setImage(data);
}

void ScrollLine::setOwnImage(void far *data)
{
	TextSprite **line = lines;

	for (int i = 0; i < 2; i++, line++)
		(*line)->setOwnImage(data);
}

int ScrollLine::getY()
{
	return lines[0]->Sprite::y;
}

int ScrollLine::imageHeight()
{
	return lines[0]->imageHeight;
}

char ScrollLine::isText()
{
	return !lines[0]->showImage;
}

char ScrollLine::isImage()
{
	return lines[0]->showImage;
}

char ScrollLine::isMarked(int n)
{
	char marked = 0;

	if (n < 2)
		marked = lines[n]->marked;
	return marked;
}

void ScrollLine::clearMarks()
{
	lines[0]->marked = 0;
	lines[1]->marked = 0;
}
