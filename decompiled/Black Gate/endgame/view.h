#ifndef VIEW_H
#define VIEW_H

#include <dos.h>

#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200
#define VIDEO_SEGMENT   0xA000

struct Point {
	int x0, y0;
	Point() {}
	void set(int x, int y) { x0 = x; y0 = y; }
	void offset(int dx, int dy) { x0 += dx; y0 += dy; }
};

/* A rectangle, corners included. */
struct Rect : Point {
	int x1, y1;
	Rect() {}
	Rect(int a, int b, int c, int d) { set(a, b, c, d); }
	void set(int a, int b, int c, int d) { Point::set(a, b); x1 = c; y1 = d; }
	int width() { return x1 - x0 + 1; }
	int height() { return y1 - y0 + 1; }
};

/* The segment a view's pixels sit in. */
struct ViewId {
	unsigned segment;
	ViewId() { segment = 0; }
};

/* A view's table of row offsets. */
struct ViewRows {
	int *rows;
	ViewRows() { rows = 0; }
	void freeRows() { if (rows) { delete rows; rows = 0; } }
	unsigned char matches(int lastX, unsigned height);
	unsigned char build(int width, unsigned height, int base);
};

/* A drawing surface and the rectangle drawing on it is clipped to. */
struct View : ViewId, ViewRows {
	Rect clip;
	int unused;
	unsigned char borrowed;         /* the pixels are not this view's to free */
	View() { borrowed = 0; }
	View(int x0, int y0, int x1, int y1) : clip(x0, y0, x1, y1) { borrowed = 0; allocate(); }
	~View() { release(); }
	View &operator=(View &v)
		{ segment = v.segment; rows = v.rows; clip.set(v.clip.x0, v.clip.y0, v.clip.x1, v.clip.y1);
		borrowed = 1; return *this; }
	void far *pixels() { return MK_FP(segment, rows[0]); }
	unsigned char allocate();
	void initScreen(unsigned char color);
	void release();
	void copyTo(View *to);
	void copyFrom(View *from);
	void clear(unsigned char color);
	void fill(Rect *r, unsigned char color);
	void fill(int x0, int y0, int x1, int y1, unsigned char color);
};

/* Makes the screen view the first time the display opens. */
struct ScreenMaker {
	int unused;
	void open();
};

extern View *CurrentView;
extern View *ScreenView;

#ifdef __cplusplus
extern "C" {
#endif
void far FillRect(View *view, int x0, int y0, int x1, int y1, unsigned char color);
void far FillView(View *view, unsigned char color);
void far DrawFrameTranslated(View *view, int x, int y, void far *shape, int frameNum, unsigned char *remap);
void far DrawEmsFrameTranslated(View *view, int x, int y, void far *shape, int frameNum, unsigned char *remap);
void far CopyView(View *from, View *to);
void far OutlineRect(View *view, int x0, int y0, int x1, int y1, unsigned char color);
void far DrawEmsFrame(View *view, int x, int y, void far *shape, int frameNum);
void far DrawFrame(View *view, int x, int y, void far *shape, int frameNum);
#ifdef __cplusplus
}
#endif

#endif
