/* Black Gate U7.EXE, resident segment 145 (file offsets 0x03f37b to 0x03f4da, 351 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "memapi.h"
#include "screen.h"

struct Rect {
	int16_t x0, y0, x1, y1;
};

/* A drawing surface: a table of row addresses and the rectangle drawing is clipped to. */
struct View {
	int16_t segment;
	int32_t rowTable;
	struct Rect clip;
};

/* Give the view's clip rectangle a pixel buffer, then a table of row addresses after it. DRAW_IN_XMS
 * takes the buffer from Voodoo's XMS block instead of the far heap. A color other than 0xff fills it. */
uint8_t AllocateDrawBuffer(struct View *view, uint8_t color, uint16_t flags)
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

	height = view->clip.y1 - view->clip.y0 + 1;
	width = view->clip.x1 - view->clip.x0 + 1;
	left = view->clip.x0;
	top = view->clip.y0;
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

void RequireDrawBuffer(struct View *view, uint8_t color, uint16_t flags)
{
	if (!AllocateDrawBuffer(view, color, flags))
		ReportError(0x2102);
}
