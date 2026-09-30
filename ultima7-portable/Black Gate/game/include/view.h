#ifndef VIEW_H
#define VIEW_H

/* The game screen, VGA mode 13h. */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

/* A rectangle, corners included. */
struct Rect {
	int16_t x0, y0, x1, y1;
};

/* A drawing surface, and the rectangle drawing on it is clipped to. */
struct View {
	int16_t segment;
	int32_t rowTable;
	Rect clip;
	View() : segment(0), rowTable(0)
	{
		Rect *r = &clip;

		r->x0 = 0;
		r->y0 = 0;
		r->x1 = 0;
		r->y1 = 0;
	}
};

extern "C" View ScreenView;
extern View Viewport;

#ifdef __cplusplus
extern "C" {
#endif
void DrawLine(View *view, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color);
void FillRectangle(View *view, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int8_t color);
#ifdef __cplusplus
}
#endif

#endif
