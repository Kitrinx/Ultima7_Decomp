/* Serpent Isle SI.EXE, resident segment 144 (file offsets 0x03ee7c to 0x03f062, 486 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <dos.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "memapi.h"
#include "screen.h"
#include "view.h"

unsigned char far pascal AllocateDrawBuffer(View *view, unsigned char color, unsigned flags)
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

	height = view->clip.bottom() - view->clip.top() + 1;
	width = view->clip.right() - view->clip.left() + 1;
	left = view->clip.left();
	top = view->clip.top();
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
	view->setSegment(0);
	view->rowTable.set(pixels + size);
	offset = 0;
	for (i = 0; i < height; i++) {
		view->rowTable.setRow(i + top, pixels + offset + left);
		offset += width;
	}
	if (color != 0xff)
		FillView(view, color);
	return 1;
}

void far RequireDrawBuffer(View *view, unsigned char color, unsigned flags)
{
	if (!AllocateDrawBuffer(view, color, flags))
		ReportError(0x2102);
}
