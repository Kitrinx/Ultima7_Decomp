#ifndef TEXTSPR_H
#define TEXTSPR_H

#include "controls.h"
#include "itable.h"

/*
 * A sprite whose shape is a font: prints its text at (x, y), aligned on x, or shows one frame of
 * an image in its place. The TextPrinter half draws into the sprite's own view.
 *
 * Text may open with a command: "\C", "\L" or "\R" aligns it, "\Pn" shows frame n of the image.
 */
struct TextSprite : Sprite, TextPrinter {
	char *text;
	int size;                   /* bytes allocated for text */
	unsigned char align;        /* ALIGN_LEFT, ALIGN_CENTRE or ALIGN_RIGHT */
	int lineWidth;
	int lineHeight;             /* height of the font, top to bottom */
	int descent;                /* rows of the font below y */
	void far *image;
	unsigned char showImage;
	int imageHeight;
	int imageFlags;
	char marked;                /* set by "#S" in the text */
	TextSprite(char *name, char back, Screen *screen, char keep);
	TextSprite(char *flexName, int entry, char back, Screen *screen, char keep);
	TextSprite(void far *font, char back, Screen *screen, char keep);
	~TextSprite();
	void setText(char far *s, int count);
	void allocateUnder();
	void drawChar(View *view, int px, int py, char c);
	void reset();
	int charHeight(char c);
	int charWidth(char c);
	void saveUnder();
	void draw();
	void restoreUnder();
	void init();
	void setImage(void far *data);
	void setOwnImage(void far *data);
	void setAlign(unsigned char a);
	char aboveTop();
	char belowBottom();
	char aboveMiddle();
	char belowMiddle();
};

void CopyLabel(TextSprite *sprite, char far *dest, char far *src, int size);
void GetShapeBounds(void far *data, Rect *bounds, int flags);

#endif
