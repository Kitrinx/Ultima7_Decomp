#ifndef VIEW_H
#define VIEW_H

#include "geometry.h"
#include "lowlevel.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

struct RowTable {
	int32_t address;
	RowTable() { address = 0; }
	int32_t set(int32_t value) { return address = value; }
	void setRow(uint16_t row, int32_t value) { SetRowAddress(row, value, get()); }
	int32_t get() { return address; }
};

struct View {
	int16_t segment;
	RowTable rowTable;
	Rect clip;
	View() : segment(0) {}
	void setSegment(int16_t value) { segment = value; }
};

extern "C" View ScreenView;
extern View Viewport;

#ifdef __cplusplus
extern "C" {
#endif
void DrawLine(View *view, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color);
#ifdef __cplusplus
}
#endif

#endif
