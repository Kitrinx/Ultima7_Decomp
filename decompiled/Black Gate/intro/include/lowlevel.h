#ifndef LOWLEVEL_H
#define LOWLEVEL_H

struct View;
struct Rect;

/* Assembly helpers: shapes, views and expanded memory, all through far pointers. */
#ifdef __cplusplus
extern "C" {
#endif

/* Expanded memory. */
void far * far MapEmsPointer(void far *p);

/* Shapes. */
int far pascal GetFrameBounds(struct Rect far *bounds, int x, int y, void far *shape, int frame);
int far pascal GetShapeFrameCount(void far *shape);
int far pascal GetMaxFrameWidth(void far *shape);
int far pascal GetMaxFrameHeight(void far *shape);
int far pascal GetEmsShapeFrameCount(void far *shape);
int far pascal GetEmsMaxFrameWidth(void far *shape);
int far pascal GetEmsMaxFrameHeight(void far *shape);

/* Drawing on views. */
void far DrawFrame(struct View *view, int x, int y, void far *shape, int frame);
void far SaveUnderFrame(struct View *view, void far *buffer, int x, int y, void far *shape, int frame);
void far RestoreUnderFrame(struct View *view, void far *buffer, int x, int y, void far *shape, int frame);
void far DrawEmsFrame(struct View *view, int x, int y, void far *shape, int frame);
void far SaveUnderEmsFrame(struct View *view, void far *buffer, int x, int y, void far *shape, int frame);
void far RestoreUnderEmsFrame(struct View *view, void far *buffer, int x, int y, void far *shape, int frame);
void far FillView(struct View *view, unsigned char color);
void far CopyView(struct View *from, struct View *to);
void far SaveRect(struct View *view, void far *buffer, struct Rect *rect);
void far RestoreRect(struct View *view, void far *buffer, struct Rect *rect);
/* The routine reads used as a far pointer; the game passes a near one. */
void far pascal MarkUsedColors(struct View *view, char *used);

/* Scale is in 256ths, angle in degrees. */
void far DrawScaledFrame(struct View *view, int x, int y, void far *shape, int frame, int angle, int scale,
	unsigned char flip);
void far DrawEmsScaledFrame(struct View *view, int x, int y, void far *shape, int frame, int angle, int scale,
	unsigned char flip);

#ifdef __cplusplus
}
#endif

#endif
