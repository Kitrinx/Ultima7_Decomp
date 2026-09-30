#ifndef MAINMENU_SCROLL_H
#define MAINMENU_SCROLL_H

namespace MainMenu {

struct TextSprite;
struct Screen;

/*
 * One line of the credits or quotes, as two text sprites: a '|' in the text puts the rest
 * in the second. Each call without a line number applies to both.
 */
struct ScrollLine {
	TextSprite *lines[2];
	ScrollLine(void *font, char back, Screen *screen, char keep);
	~ScrollLine();
	void moveTo(int16_t x, int16_t y);
	void moveTo(int16_t x, int16_t y, int16_t n);
	void moveBy(int16_t dx, int16_t dy);
	void moveBy(int16_t dx, int16_t dy, int16_t n);
	uint8_t aboveTop();
	uint8_t aboveMiddle();
	char aboveTop(int16_t n);
	char aboveMiddle(int16_t n);
	void setText(char *text);
	void setText(char *text, int16_t n);
	void setAlign(char a);
	void setSpacing(int16_t spacing);
	void setImage(void *data);
	void setOwnImage(void *data);
	int16_t getY();
	int16_t imageHeight();
	char isText();
	char isImage();
	char isMarked(int16_t n);
	void clearMarks();
};

}

#endif
