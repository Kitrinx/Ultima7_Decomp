#ifndef SCROLL_H
#define SCROLL_H

namespace MainMenu {

struct TextSprite;
struct Screen;

/*
 * One line of the credits or quotes, as two text sprites: a '|' in the text puts the rest
 * in the second. Each call without a line number applies to both.
 */
struct ScrollLine {
	TextSprite *lines[2];
	ScrollLine(void *font, int8_t back, Screen *screen, int8_t keep);
	~ScrollLine();
	void moveTo(int16_t x, int16_t y);
	void moveTo(int16_t x, int16_t y, int16_t n);
	void moveBy(int16_t dx, int16_t dy);
	void moveBy(int16_t dx, int16_t dy, int16_t n);
	uint8_t aboveTop();
	uint8_t aboveMiddle();
	int8_t aboveTop(int16_t n);
	int8_t aboveMiddle(int16_t n);
	void setText(char *text);
	void setText(char *text, int16_t n);
	void setAlign(int8_t a);
	void setSpacing(int16_t spacing);
	void setImage(void *data);
	void setOwnImage(void *data);
	int16_t getY();
	int16_t imageHeight();
	int8_t isText();
	int8_t isImage();
	int8_t isMarked(int16_t n);
	void clearMarks();
};

}

#endif
