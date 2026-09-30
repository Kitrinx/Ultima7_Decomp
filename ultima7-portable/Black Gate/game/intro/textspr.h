#ifndef INTRO_TEXTSPR_H
#define INTRO_TEXTSPR_H

#include "itable.h"
#include "controls.h"

namespace Intro {

/* How text stands on its x. */
#define ALIGN_LEFT      0
#define ALIGN_CENTRE    1
#define ALIGN_RIGHT     2

/*
 * A sprite whose shape is a font: prints its text at (x, y), aligned on x. The TextPrinter half
 * draws into the sprite's own view.
 */
struct TextSprite : Sprite, TextPrinter {
	char *text;
	int16_t size;               /* bytes allocated for text */
	uint8_t align;              /* ALIGN_LEFT, ALIGN_CENTRE or ALIGN_RIGHT */
	int16_t lineWidth;
	int16_t lineHeight;         /* height of the font, top to bottom */
	int16_t descent;            /* rows of the font below y */
	TextSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
		: Sprite(flexName, entry, back, screen, keep)
	{
		init();
		measure();
	}
	~TextSprite();
	void setText(char *s);
	void measure();
	void drawChar(View *view, int16_t px, int16_t py, int8_t c);
	void reset();
	int16_t charHeight(int8_t c);
	int16_t charWidth(int8_t c);
	void saveUnder();
	void draw();
	void restoreUnder();
	void init();
};

void GetShapeBounds(int32_t shape, Rect *bounds);

}

#endif
