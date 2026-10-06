#ifndef GEOMETRY_H
#define GEOMETRY_H

struct Point {
	int x, y;
	Point(int a, int b) { set(a, b); }
	Point *set(int a, int b) { x = a; y = b; return this; }
};

struct Rect : Point {
	int x1, y1;
	Rect() : Point(0, 0) { x1 = 0; y1 = 0; }
	int bottom() { return y1; }
	int top() { return y; }
	int right() { return x1; }
	int left() { return x; }
	Rect *set(int left, int top, int right, int bottom)
	{
		x = left;
		y = top;
		x1 = right;
		y1 = bottom;
		return this;
	}
};

#endif
