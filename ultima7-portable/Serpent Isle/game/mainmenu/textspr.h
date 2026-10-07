#ifndef TEXTSPR_H
#define TEXTSPR_H

#include "controls.h"
#include "../shared/itable.h"

namespace MainMenu {

using Shared::TextPrinter;

/*
 * A sprite whose shape is a font: prints its text at (x, y), aligned on x, or shows one frame of
 * an image in its place. The TextPrinter half draws into the sprite's own view.
 *
 * Text may open with a command: "\C", "\L" or "\R" aligns it, "\Pn" shows frame n of the image.
 */
struct TextSprite : Sprite, TextPrinter {
	char *text;
	int16_t size;                   /* bytes allocated for text */
	uint8_t align;        /* ALIGN_LEFT, ALIGN_CENTRE or ALIGN_RIGHT */
	int16_t lineWidth;
	int16_t lineHeight;             /* height of the font, top to bottom */
	int16_t descent;                /* rows of the font below y */
	void *image;
	uint8_t showImage;
	int16_t imageHeight;
	int16_t imageFlags;
	int8_t marked;                /* set by "#S" in the text */
	TextSprite(char *name, int8_t back, Screen *screen, int8_t keep);
	TextSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	TextSprite(void *font, int8_t back, Screen *screen, int8_t keep);
	~TextSprite();
	void setText(char *s, int16_t count);
	void allocateUnder();
	void drawChar(View *view, int16_t px, int16_t py, int8_t c);
	void reset();
	int16_t charHeight(int8_t c);
	int16_t charWidth(int8_t c);
	void saveUnder();
	void draw();
	void restoreUnder();
	void init();
	void setImage(void *data);
	void setOwnImage(void *data);
	void setAlign(uint8_t a);
	int8_t aboveTop();
	int8_t belowBottom();
	int8_t aboveMiddle();
	int8_t belowMiddle();
	int16_t getImageHeight() { return imageHeight; }
	int8_t isText() { return !showImage; }
	int8_t isImage() { return showImage; }
	int8_t isMarked() { return marked; }
	void clearMark() { marked = 0; }
};

void CopyLabel(TextSprite *sprite, char *dest, char *src, int16_t size);
void GetShapeBounds(void *data, Rect *bounds, int16_t flags);

}

#endif
