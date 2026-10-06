#ifndef SCROLL_H
#define SCROLL_H

struct TextSprite;
struct Screen;

/*
 * One line of the credits or quotes, as two text sprites: a '|' in the text puts the rest
 * in the second. Each call without a line number applies to both.
 */
struct ScrollLine {
	TextSprite *lines[2];
	ScrollLine(void far *font, char back, Screen *screen, char keep);
	~ScrollLine();
	void moveTo(int x, int y);
	void moveTo(int x, int y, int n);
	void moveBy(int dx, int dy);
	void moveBy(int dx, int dy, int n);
	unsigned char aboveTop();
	unsigned char aboveMiddle();
	char aboveTop(int n);
	char aboveMiddle(int n);
	void setText(char far *text);
	void setText(char far *text, int n);
	void setAlign(char a);
	void setSpacing(int spacing);
	void setImage(void far *data);
	void setOwnImage(void far *data);
	int getY();
	int imageHeight();
	char isText();
	char isImage();
	char isMarked(int n);
	void clearMarks();
};

#endif
