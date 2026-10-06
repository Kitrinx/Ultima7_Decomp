/* Serpent Isle ENDGAME.EXE, resident segment 21 (file offsets 0x00bd96 to 0x00bf9f, 521 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "view.h"
#include "memsys.h"

/* Gives the view pixels of its own in far memory, one byte per pixel of the clip box. Returns 1 when
 * it has them. */
unsigned char View::allocate()
{
	void far *pixels;
	int width, height;

	borrowed = 1;
	width = clip.width();
	height = clip.height();
	long size = (unsigned)(height * width);
	if ((size >> 16) == 0) {        /* fits in one segment */
		if ((pixels = Memory.allocate(size, FAR_MEMORY, 2, 0)) != 0) {
			if (build(width, height, FP_OFF(pixels))) {
				segment = FP_SEG(pixels);
				borrowed = 0;
			} else
				Memory.release(&pixels, FAR_MEMORY);
		}
	}
	return !borrowed;
}

/* Makes this the whole screen, cleared to color. */
void View::initScreen(unsigned char color)
{
	clip.set(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	segment = VIDEO_SEGMENT;
	build(clip.width(), clip.height(), 0);
	clear(color);
}

/* Frees the view's own pixels and its row table. */
void View::release()
{
	void far *pixels;

	if (!borrowed) {
		freeRows();
		/* rows is null by now, so the offset comes from the start of DGROUP */
		Memory.release(&(pixels = MK_FP(segment, *rows)), FAR_MEMORY);
	}
}

void View::copyTo(View *to)
{
	if (segment)
		CopyView(this, to);
}

void View::copyFrom(View *from)
{
	if (segment)
		CopyView(from, this);
}

void View::clear(unsigned char color)
{
	if (segment)
		FillView(this, color);
}

void View::fill(Rect *r, unsigned char color)
{
	if (segment)
		FillRect(this, r->x0, r->y0, r->x1, r->y1, color);
}

void View::fill(int x0, int y0, int x1, int y1, unsigned char color)
{
	if (segment)
		FillRect(this, x0, y0, x1, y1, color);
}
