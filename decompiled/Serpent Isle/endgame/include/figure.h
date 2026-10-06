#ifndef FIGURE_H
#define FIGURE_H

#include "view.h"

/* The color boxes are drawn in; set by each box's set(). */
extern unsigned char FigureColor;

/* Something drawn on the current view that can be moved. */
struct Figure {
	virtual void draw() = 0;
	virtual void erase() = 0;
	virtual void moveBy(int dx, int dy) = 0;
};

struct Shape : Figure, Point {
	Shape() {}
	void draw() {}
	void erase() {}
	void moveBy(int dx, int dy) { erase(); offset(dx, dy); draw(); }
	virtual void moveTo(int x, int y) { Point::set(x, y); draw(); }
};

struct Marker : Shape {
	int width, height;
	void draw() {}
};

/* A rectangle outline from (x0, y0) to (x1, y1), drawn on the current view. */
struct Box : Marker {
	int x1, y1;
	void draw();
	virtual void set(int x0, int y0, int x1, int y1, unsigned char color);
};

/* A box that is also filled on a given view. */
struct FramedBox : Box {
	void fill(View *view);
	virtual void drawFramed(View *view);
	virtual void set(int x0, int y0, int x1, int y1, unsigned char color, View *view);
};

#endif
