#ifndef VIEW_H
#define VIEW_H

#include "lowlevel.h"

/* The game screen, VGA mode 13h. */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

struct Point {
	int x0, y0;
	Point() { x0 = 0; y0 = 0; }
	Point(int x, int y) { set(x, y); }
	int getX() { return x0; }
	int getY() { return y0; }
	void setX(int x) { x0 = x; }
	void setY(int y) { y0 = y; }
	Point &set(int x, int y) { x0 = x; y0 = y; return *this; }
};

/* A rectangle, corners included. */
struct Rect : Point {
	int x1, y1;
	Rect() : Point(0, 0) { x1 = 0; y1 = 0; }
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	int getX1() { return x1; }
	int getY1() { return y1; }
	void setX1(int x) { x1 = x; }
	void setY1(int y) { y1 = y; }
	int getLeft() { return x0; }
	int getTop() { return y0; }
	int getRight() { return x1; }
	int getBottom() { return y1; }
	int getWidth() { return getRight() - getLeft() + 1; }
	int getHeight() { return getBottom() - getTop() + 1; }
	Rect &setBounds(int a, int b, int c, int d)
	{
		x0 = a; y0 = b; x1 = c; y1 = d;
		return *this;
	}
	void set(int a, int b, int c, int d) { setBounds(a, b, c, d); }
	Rect &set(int x, int y)
	{
		x1 = x + (x1 - x0);
		y1 = y + (y1 - y0);
		x0 = x;
		y0 = y;
		return *this;
	}
	int contains(int x, int y) { return (x >= x0) & (x <= x1) & (y >= y0) & (y <= y1); }
};

/* A view's handle. */
struct ViewId {
	int id;
	ViewId(int value = 0) { id = value; }
};

/* Where a view's table of row addresses sits in flat memory. */
struct ViewPixels {
	long rowTable;
	ViewPixels() { rowTable = 0; }
};

/* A drawing surface, and the rectangle drawing on it is clipped to. */
struct View : ViewId, ViewPixels {
	Rect clip;
	View() {}
	void copy(View *view) { *this = *view; }
	View &copyFrom(View *view) { CopyView(view, this); return *this; }
	void fill(unsigned char color) { FillView(this, color); }
};

void GetShapeBounds(void far *data, Rect *bounds, int flags);

extern View ScreenView;
extern View Viewport;

#ifdef __cplusplus
extern "C" {
#endif
void far DrawLine(View *view, int x0, int y0, int x1, int y1, char color);
void far FillRectangle(View *view, int x0, int y0, int x1, int y1, char color);
#ifdef __cplusplus
}
#endif

#endif
