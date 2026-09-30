#ifndef TEXTSPR_H
#define TEXTSPR_H

#include "controls.h"
#include "itable.h"

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
	int size;                   /* bytes allocated for text */
	unsigned char align;        /* ALIGN_LEFT, ALIGN_CENTRE or ALIGN_RIGHT */
	int lineWidth;
	int lineHeight;             /* height of the font, top to bottom */
	int descent;                /* rows of the font below y */
	TextSprite(char *flexName, int entry, char back, Screen *screen, char keep)
		: Sprite(flexName, entry, back, screen, keep)
	{
		init();
		measure();
	}
	~TextSprite();
	void setText(char far *s);
	void measure();
	void drawChar(View *view, int px, int py, char c);
	void reset();
	int charHeight(char c);
	int charWidth(char c);
	void saveUnder();
	void draw();
	void restoreUnder();
	void init();
};

void GetShapeBounds(void far *data, Rect *bounds);

#endif
