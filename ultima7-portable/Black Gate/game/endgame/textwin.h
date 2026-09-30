#ifndef ENDGAME_TEXTWIN_H
#define ENDGAME_TEXTWIN_H

#include "view.h"

namespace Endgame {

/* justify values: where each line starts */
#define JUSTIFY_LEFT    0
#define JUSTIFY_RIGHT   1
#define JUSTIFY_CENTER  2
#define JUSTIFY_FULL    3

/* A font drawn from a shape file whose frames are the characters. */
struct ShapeFont {
	int32_t shape;
	int16_t baseline;
	int16_t spacing;
	int16_t leading;
	int16_t spaceWidth;
	ShapeFont(int32_t shapes);
	void setSpacing(int16_t space, int16_t gap, int16_t lines);
	void drawChar(::View *view, uint8_t c, int16_t x, int16_t y);
	int16_t charWidth(uint8_t c);
	int16_t charHeight();
};

/* Text printed into a copy of a view with a font; format codes place it. */
struct TextWindow {
	::View view;
	int16_t cursorX, cursorY;
	ShapeFont *font;
	uint8_t justify;
	uint8_t foreground;
	uint8_t background;
	TextWindow(::View *where, ShapeFont *f);
	virtual ~TextWindow() {}
	virtual int16_t left() { return view.clip.x0; }
	virtual int16_t top() { return view.clip.y0; }
	virtual int16_t right() { return view.clip.x1; }
	int16_t column() { return cursorX - left(); }
	int16_t row() { return cursorY - top(); }
	void setX(int16_t x) { cursorX = left() + x; }
	void setY(int16_t y) { cursorY = top() + y; }
	void putChar(char c);
	void putLines(char *text);
	void format(const char *fmt, va_list args);
	void print(const char *fmt, ...);
	int16_t measureLine(char *text);
	void applyJustify(char *text);
};

/* A TextWindow confined to its own margins inside the view. */
struct TextBox : TextWindow {
	::Rect margins;
	TextBox(::Rect r, ::View *where, ShapeFont *f) : TextWindow(where, f) { margins = r; }
	int16_t left() { return margins.x0; }
	int16_t top() { return margins.y0; }
	int16_t right() { return margins.x1; }
};

}

#endif
