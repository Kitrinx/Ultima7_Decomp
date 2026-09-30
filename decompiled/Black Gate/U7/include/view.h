#ifndef VIEW_H
#define VIEW_H

/* The game screen, VGA mode 13h. */
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   200

/* A rectangle, corners included. */
struct Rect {
	int x0, y0, x1, y1;
};

/* A drawing surface, and the rectangle drawing on it is clipped to. */
struct View {
	int segment;
	long rowTable;
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
