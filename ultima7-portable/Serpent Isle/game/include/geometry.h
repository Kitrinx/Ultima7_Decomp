#ifndef GEOMETRY_H
#define GEOMETRY_H

struct Point {
	int16_t x, y;
	Point(int16_t a, int16_t b) { set(a, b); }
	Point *set(int16_t a, int16_t b) { x = a; y = b; return this; }
};

struct Rect : Point {
	int16_t x1, y1;
	Rect() : Point(0, 0) { x1 = 0; y1 = 0; }
	int16_t bottom() { return y1; }
	int16_t top() { return y; }
	int16_t right() { return x1; }
	int16_t left() { return x; }
	Rect *set(int16_t left, int16_t top, int16_t right, int16_t bottom)
	{
		x = left;
		y = top;
		x1 = right;
		y1 = bottom;
		return this;
	}
};

#endif
