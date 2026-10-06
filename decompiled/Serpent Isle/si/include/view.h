#ifndef VIEW_H
#define VIEW_H

#include "geometry.h"
#include "lowlevel.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

struct RowTable {
	long address;
	RowTable() { address = 0; }
	long set(long value) { return address = value; }
	void setRow(unsigned row, long value) { SetRowAddress(row, value, get()); }
	long get() { return address; }
};

struct View {
	int segment;
	RowTable rowTable;
	Rect clip;
	View() : segment(0) {}
	void setSegment(int value) { segment = value; }
};

extern View ScreenView;
extern View Viewport;

#ifdef __cplusplus
extern "C" {
#endif
void far DrawLine(View *view, int x0, int y0, int x1, int y1, char color);
#ifdef __cplusplus
}
#endif

#endif
