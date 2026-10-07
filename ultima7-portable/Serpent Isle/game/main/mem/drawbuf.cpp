/* Serpent Isle SI.EXE, resident segment 144 (file offsets 0x03ee7c to 0x03f062, 486 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "memapi.h"
#include "screen.h"
#include "view.h"

uint8_t AllocateDrawBuffer(View *view, uint8_t color, uint16_t flags)
{
	void *memory;
	int32_t pixels;
	int32_t offset;
	int32_t size;
	uint16_t height;
	uint16_t width;
	uint16_t left;
	uint16_t top;
	uint16_t i;

	height = view->clip.bottom() - view->clip.top() + 1;
	width = view->clip.right() - view->clip.left() + 1;
	left = view->clip.left();
	top = view->clip.top();
	size = (uint32_t) height * width;
	if (flags & DRAW_IN_XMS) {
		pixels = AllocateVoodooMemory(&VoodooXmsBlock, size + height * sizeof(int32_t));
		if (pixels == 0)
			return 0;
	} else {
		memory = AllocateFarHeap(size + height * sizeof(int32_t), flags | FAR_ALIGN_PARA);
		if (memory == 0)
			return 0;
		pixels = PointerToLinear(memory);
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

void RequireDrawBuffer(View *view, uint8_t color, uint16_t flags)
{
	if (!AllocateDrawBuffer(view, color, flags))
		ReportError(0x2102);
}
