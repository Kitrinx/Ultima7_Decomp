#ifndef VIEW_H
#define VIEW_H

/* The game screen, VGA mode 13h. */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

struct Point {
	int x0, y0;
	Point() { x0 = 0; y0 = 0; }
	Point(int x, int y) { x0 = x; y0 = y; }
};

/* A rectangle, corners included. */
struct Rect : Point {
	int x1, y1;
	Rect() : Point() { x1 = 0; y1 = 0; }
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	int contains(int x, int y) { return (x >= x0) & (x <= x1) & (y >= y0) & (y <= y1); }
};

/* A view's handle. */
struct ViewId {
	int id;
	ViewId() { id = 0; }
};

/* Where a view's table of row addresses sits in flat memory. */
struct ViewPixels {
	long rowTable;
	ViewPixels() { rowTable = 0; }
};

/* A drawing surface, and the rectangle drawing on it is clipped to. */
struct View : ViewId, ViewPixels {
	Rect clip;
};

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
