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
};

/* The video segment a view draws into. */
struct ViewId {
	int id;
	ViewId() { id = 0; }
};

/* A near table of each row's offset in the view's segment. */
struct ViewRows {
	unsigned *rows;
	ViewRows() { rows = 0; }
};

/* A drawing surface, and the rectangle drawing on it is clipped to. */
struct View : ViewId, ViewRows {
	Rect clip;
	View() {}
	View(int x0, int y0, int x1, int y1) : clip(x0, y0, x1, y1) {}
};

extern View ScreenView;
extern View Viewport;

#endif
