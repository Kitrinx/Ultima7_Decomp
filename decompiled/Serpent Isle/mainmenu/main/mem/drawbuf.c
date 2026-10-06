/* Serpent Isle MAINMENU.EXE, one module of resident segment 58 (file offsets 0x01727f to 0x0173de, 351 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include <dos.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "memapi.h"
#include "screen.h"

struct Rect {
	int x0, y0, x1, y1;
};

/* A drawing surface: a table of row addresses and the rectangle drawing is clipped to. */
struct View {
	int segment;
	long rowTable;
	struct Rect clip;
};

/* Give the view's clip rectangle a pixel buffer, then a table of row addresses after it. DRAW_IN_XMS
 * takes the buffer from Voodoo's XMS block instead of the far heap. A color other than 0xff fills it. */
unsigned char far pascal AllocateDrawBuffer(struct View *view, unsigned char color, unsigned flags)
{
	void far *memory;
	long pixels;
	long offset;
	long size;
	unsigned height;
	unsigned width;
	unsigned left;
	unsigned top;
	unsigned i;

	height = view->clip.y1 - view->clip.y0 + 1;
	width = view->clip.x1 - view->clip.x0 + 1;
	left = view->clip.x0;
	top = view->clip.y0;
	size = (unsigned long) height * width;
	if (flags & DRAW_IN_XMS) {
		pixels = AllocateVoodooMemory(&VoodooXmsBlock, size + height * sizeof(long));
		if (pixels == 0)
			return 0;
	} else {
		memory = AllocateFarHeap(size + height * sizeof(long), flags | FAR_ALIGN_PARA);
		if (memory == 0)
			return 0;
		pixels = ((unsigned long) FP_SEG(memory) << 4) + FP_OFF(memory);
	}
	view->segment = 0;
	view->rowTable = pixels + size;
	offset = 0;
	for (i = 0; i < height; i++) {
		SetRowAddress(i + top, pixels + offset + left, view->rowTable);
		offset += width;
	}
	if (color != 0xff)
		FillView(view, color);
	return 1;
}

void far RequireDrawBuffer(struct View *view, unsigned char color, unsigned flags)
{
	if (!AllocateDrawBuffer(view, color, flags))
		ReportError(0x2102);
}
