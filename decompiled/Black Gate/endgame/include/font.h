#ifndef FONT_H
#define FONT_H

#include "memsys.h"
#include "view.h"

/* A font: glyph shapes and spacing. */
struct Font {
	MemHandle shape;
	int unknown;
	int baseline;
	int spacing;
	int leading;
	int spaceWidth;
	Font() {}
	virtual ~Font() {}
	virtual void drawChar(View *view, unsigned char c, int x, int y) = 0;
	virtual int setColor(unsigned char color) = 0;
	virtual int setBackground(unsigned char color) = 0;
	virtual int setColors(unsigned char *colors) = 0;
	virtual int setShadow(unsigned char color) = 0;
	virtual int charWidth(unsigned char c) = 0;
	virtual int charOverhang(unsigned char c) = 0;
	virtual int charHeight() = 0;
	virtual int ascent() = 0;
	void setSpacing(int space, int gap, int lines)
	{
		spacing = gap;
		leading = lines;
		if (space == -1)
			spaceWidth = charWidth('-');
		else
			spaceWidth = space;
	}
};

/* A font drawn from a shape file, optionally recoloured through a map. */
struct ShapeFont : Font {
	unsigned char *colorMap;
	ShapeFont(MemHandle &shapes);
	~ShapeFont();
	void init(MemHandle &shapes);
	void drawChar(View *view, unsigned char c, int x, int y);
	int setColor(unsigned char color);
	int setBackground(unsigned char color);
	int setColors(unsigned char *colors);
	int setShadow(unsigned char color);
	int charWidth(unsigned char c);
	int charOverhang(unsigned char c);
	int charHeight();
	int ascent();
};

extern "C" void pascal GetFrameBounds(Rect far *bounds, int x, int y, void far *shape, int frameNum);

#endif
