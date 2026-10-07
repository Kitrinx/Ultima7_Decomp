/* Serpent Isle MAINMENU.EXE, resident segment 18 (file offsets 0x00f245 to 0x00f664, 1055 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 * Accessor names and no-inline setting inferred from emitted methods and calls.
 */

#include "u7port.h"
#include "oops.h"
#include "textspr.h"
#include "scroll.h"

namespace MainMenu {

#define LINE_BREAK  '|'
#define WHOLE_TEXT  -1      /* setText measures the string itself */

ScrollLine::ScrollLine(void *font, int8_t back, Screen *screen, int8_t keep)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++) {
		if ((*line = new TextSprite(font, back, screen, keep)) == 0)
			ReportOutOfVoodooMemory();
	}
}

ScrollLine::~ScrollLine()
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		if (*line)
			delete *line;
}

void ScrollLine::moveTo(int16_t x, int16_t y)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->moveTo(x, y);
}

void ScrollLine::moveTo(int16_t x, int16_t y, int16_t n)
{
	if (n < 2)
		lines[n]->moveTo(x, y);
}

void ScrollLine::moveBy(int16_t dx, int16_t dy)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->moveBy(dx, dy);
}

void ScrollLine::moveBy(int16_t dx, int16_t dy, int16_t n)
{
	if (n < 2)
		lines[n]->moveBy(dx, dy);
}

/* True when both lines have scrolled off the top. */
uint8_t ScrollLine::aboveTop()
{
	int8_t above = 1;
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		above = above && (*line)->aboveTop();
	return above;
}

uint8_t ScrollLine::aboveMiddle()
{
	int8_t above = 1;
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		above = above && (*line)->aboveMiddle();
	return above;
}

int8_t ScrollLine::aboveTop(int16_t n)
{
	if (n < 2)
		return lines[n]->aboveTop();
	return 0;               /* not reached: callers pass 0 */
}

int8_t ScrollLine::aboveMiddle(int16_t n)
{
	if (n < 2)
		return lines[n]->aboveMiddle();
	return 0;               /* not reached: callers pass 0 */
}

void ScrollLine::setText(char *text)
{
	char *p = text;
	int16_t length = 0;

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

void ScrollLine::setText(char *text, int16_t n)
{
	if (n < 2) {
		lines[n]->show();
		lines[n]->setText(text, WHOLE_TEXT);
	}
}

void ScrollLine::setAlign(int8_t a)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->setAlign(a);
}

void ScrollLine::setSpacing(int16_t spacing)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->setSpacing(spacing);
}

void ScrollLine::setImage(void *data)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->setImage(data);
}

void ScrollLine::setOwnImage(void *data)
{
	TextSprite **line = lines;

	for (int16_t i = 0; i < 2; i++, line++)
		(*line)->setOwnImage(data);
}

int16_t ScrollLine::getY()
{
	return lines[0]->Sprite::y;
}

int16_t ScrollLine::imageHeight()
{
	return lines[0]->getImageHeight();
}

int8_t ScrollLine::isText()
{
	return lines[0]->isText();
}

int8_t ScrollLine::isImage()
{
	return lines[0]->isImage();
}

int8_t ScrollLine::isMarked(int16_t n)
{
	int8_t marked = 0;

	if (n < 2)
		marked = lines[n]->isMarked();
	return marked;
}

void ScrollLine::clearMarks()
{
	lines[0]->clearMark();
	lines[1]->clearMark();
}

}
