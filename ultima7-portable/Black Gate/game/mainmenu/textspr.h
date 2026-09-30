#ifndef MAINMENU_TEXTSPR_H
#define MAINMENU_TEXTSPR_H

#include "controls.h"
#include "../shared/itable.h"

namespace MainMenu {

/*
 * A sprite whose shape is a font: prints its text at (x, y), aligned on x, or shows one frame of
 * an image in its place. The TextPrinter half draws into the sprite's own view.
 *
 * Text may open with a command: "\C", "\L" or "\R" aligns it, "\Pn" shows frame n of the image.
 */
struct TextSprite : Sprite, TextPrinter {
	char *text;
	int16_t size;               /* bytes allocated for text */
	uint8_t align;              /* ALIGN_LEFT, ALIGN_CENTRE or ALIGN_RIGHT */
	int16_t lineWidth;
	int16_t lineHeight;         /* height of the font, top to bottom */
	int16_t descent;            /* rows of the font below y */
	void *image;
	uint8_t showImage;
	int16_t imageHeight;
	int16_t imageFlags;
	char marked;                /* set by "#S" in the text */
	TextSprite(char *name, char back, Screen *screen, char keep);
	TextSprite(char *flexName, int16_t entry, char back, Screen *screen, char keep);
	TextSprite(void *font, char back, Screen *screen, char keep);
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
	char aboveTop();
	char belowBottom();
	char aboveMiddle();
	char belowMiddle();
};

void CopyLabel(TextSprite *sprite, char *dest, char *src, int16_t size);
void GetShapeBounds(int32_t data, Rect *bounds, int16_t flags);

}

#endif
