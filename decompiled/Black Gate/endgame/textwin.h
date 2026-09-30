#ifndef TEXTWIN_H
#define TEXTWIN_H

#include <stdarg.h>
#include "view.h"
#include "figure.h"

struct Font;

/* justify values: where each line starts */
#define JUSTIFY_LEFT    0
#define JUSTIFY_RIGHT   1
#define JUSTIFY_CENTER  2
#define JUSTIFY_FULL    3

/* A colour argument meaning "the window's background". */
#define BACKGROUND      0xFF

/* Text printed into a copy of a view with a font; format codes place and colour it. */
struct TextWindow {
	View view;
	Point cursor;
	Font *font;
	unsigned char justify;
	unsigned char foreground;
	unsigned char background;
	TextWindow(View *where, Font *f);
	virtual int left() { return view.clip.x0; }
	virtual int top() { return view.clip.y0; }
	virtual int right() { return view.clip.x1; }
	virtual int bottom() { return view.clip.y1; }
	virtual void setClip(int x0, int y0, int x1, int y1);
	virtual void setHorizontal(int x0, int x1);
	virtual void setVertical(int y0, int y1);
	virtual void clear(unsigned char color);
	void setFont(Font *f) { font = f; }
	unsigned char hasFont() { return font != 0; }
	int column() { return cursor.x0 - left(); }
	int row() { return cursor.y0 - top(); }
	void setX(int x) { cursor.x0 = left() + x; }
	void setY(int y) { cursor.y0 = top() + y; }
	void moveTo(int x, int y) { setX(x); setY(y); }
	void home() { moveTo(0, 0); }
	unsigned char setForeground(unsigned char color);
	unsigned char setBackground(unsigned char color);
	void pascal erase(unsigned char color);
	void pascal eraseLine(unsigned char color);
	void pascal eraseBelow(unsigned char color);
	void putChar(char c);
	void putLines(char *text);
	void format(char *fmt, va_list args);
	void print(char *fmt, ...);
	void pause();
	int measureLine(char *text);
	void applyJustify(char *text);
};

/* A TextWindow confined to its own margins inside the view. */
struct TextBox : TextWindow {
	Rect margins;
	TextBox(Rect r, View *where, Font *f) : TextWindow(where, f)
		{ margins.set(r.x0, r.y0, r.x1, r.y1); }
	int left() { return margins.x0; }
	int top() { return margins.y0; }
	int right() { return margins.x1; }
	int bottom() { return margins.y1; }
	void setClip(int x0, int y0, int x1, int y1) { margins.set(x0, y0, x1, y1); home(); }
	void setHorizontal(int x0, int x1) { margins.x0 = x0; margins.x1 = x1; home(); }
	void setVertical(int y0, int y1) { margins.y0 = y0; margins.y1 = y1; home(); }
	void clear(unsigned char color)
	{
		FramedBox box;
		box.set(margins.x0, margins.y0, margins.x1, margins.y1, color, &view);
	}
};

#endif
