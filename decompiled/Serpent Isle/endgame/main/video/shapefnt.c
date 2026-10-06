/* Serpent Isle ENDGAME.EXE, resident segment 12 (file offsets 0x00abbb to 0x00af89, 974 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "font.h"

#define COLORS  256

/* Takes over the shapes and sets the spacing from the capital A. */
void ShapeFont::init(MemHandle &shapes)
{
	shape.copy(shapes);
	shape.takeFrom(shapes);
	baseline = ascent();
	colorMap = 0;
	setSpacing(-1, 2, 0);
}

ShapeFont::ShapeFont(MemHandle &shapes)
{
	init(shapes);
}

/* The height of a capital, from the top of 'A' to the baseline. */
int ShapeFont::charHeight()
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.pointer(), 'A');
	return abs(bounds.y0) + 1;
}

int ShapeFont::ascent()
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.pointer(), 'A');
	return abs(bounds.y0) + 1;
}

void ShapeFont::drawChar(View *view, unsigned char c, int x, int y)
{
	if (colorMap)
		DrawFrameTranslated(view, x, y + baseline, shape.pointer(), c, colorMap);
	else
		DrawFrame(view, x, y + baseline, shape.pointer(), c);
}

int ShapeFont::charWidth(unsigned char c)
{
	Rect bounds;

	if (c == ' ')
		return spaceWidth;
	else {
		GetFrameBounds(&bounds, 0, 0, shape.pointer(), c);
		return bounds.x1 + 1;
	}
}

/* How far the glyph reaches left of where it is drawn. */
int ShapeFont::charOverhang(unsigned char c)
{
	Rect bounds;

	GetFrameBounds(&bounds, 0, 0, shape.pointer(), c);
	return abs(bounds.x0);
}

/* Draws every pixel of every glyph in one colour. */
int ShapeFont::setColor(unsigned char color)
{
	int i;

	if (colorMap == 0)
		colorMap = new unsigned char[COLORS];
	for (i = 0; i < COLORS; i++)
		colorMap[i] = color;
	return 0;
}

int ShapeFont::setBackground(unsigned char color)
{
	return 0;
}

int ShapeFont::setColors(unsigned char *colors)
{
	int i;

	if (colorMap == 0)
		colorMap = new unsigned char[COLORS];
	for (i = 0; i < COLORS; i++)
		colorMap[i] = *colors;
	return 0;
}

int ShapeFont::setShadow(unsigned char color)
{
	return 0;
}

/* The colour map is not freed; the handle's own destructor frees the shapes. */
ShapeFont::~ShapeFont()
{
	if (shape.pointer())
		;
}
