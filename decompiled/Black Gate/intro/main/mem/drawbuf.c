/* Black Gate INTRO.EXE, resident segment 42 (file offsets 0x011145 to 0x0113c1, 636 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include <dos.h>
#include <alloc.h>
#include "lowlevel.h"
#include "dosio.h"
#include "memapi.h"
#include "screen.h"
#include "vidpage.h"

#define VGA_SEGMENT     0xa000
#define PAGE_SIZE       256         /* bytes in one page of video memory */
#define NO_PAGE         65535

struct Rect {
	int x0, y0, x1, y1;
};

/* A drawing surface: its segment, a near table of row offsets, and the rectangle drawing is
 * clipped to. */
struct View {
	int segment;
	unsigned *rows;
	struct Rect clip;
};

extern char DisplayMode;

/* Give the view's clip rectangle a pixel buffer and a table of row offsets. In EGA mode the
 * buffer is spare video memory; otherwise it comes from the far heap. A color other than 0xff
 * fills it. */
unsigned char far pascal AllocateDrawBuffer(struct View *view, unsigned char color, unsigned flags)
{
	char huge *memory;
	long size;
	unsigned rowBytes;
	unsigned offset;
	unsigned height;
	unsigned width;
	unsigned left;
	unsigned top;
	unsigned i;

	height = view->clip.y1 - view->clip.y0 + 1;
	width = view->clip.x1 - view->clip.x0 + 1;
	left = view->clip.x0;
	top = view->clip.y0;
	switch (DisplayMode) {
	case 1:
		rowBytes = width >> 3;
		if ((view->clip.x0 & 7) || (width & 7))
			rowBytes += 2;
		break;
	case 2:
	case 4:
		rowBytes = (width >> 2) + 2;
		break;
	case 3:
		rowBytes = (width >> 1) + 2;
		break;
	default:
		rowBytes = width;
		break;
	}
	switch (DisplayMode) {
	case 1:
		i = AllocatePageRun((rowBytes * height + PAGE_SIZE) >> 8) << 8;
		if ((long) i == NO_PAGE)
			return 0;
		memory = (char huge *) MK_FP(VGA_SEGMENT, 0);
		memory += i;
		break;
	case 2:
	case 3:
	case 4:
		size = (unsigned long) height * rowBytes;
		memory = AllocateFarHeap(size, flags | FAR_ALIGN_PARA);
		if (!memory)
			return 0;
		break;
	default:
		size = (unsigned long) height * width;
		memory = AllocateFarHeap(size, flags | FAR_ALIGN_PARA);
		if (!memory)
			return 0;
		break;
	}
	view->segment = FP_SEG(memory);
	offset = FP_OFF(memory);
	view->rows = (unsigned *) malloc((height + top) * sizeof(unsigned));
	if (view->rows == 0) {
		if (DisplayMode == 1)
			FreePageRun((unsigned) (view->segment - (int) VGA_SEGMENT) >> 4);
		else
			FreeFarHeap(memory);
		return 0;
	}
	for (i = 0; i < height; i++) {
		switch (DisplayMode) {
		case 1:
			view->rows[i + top] = offset - (left >> 3);
			break;
		case 2:
		case 4:
			view->rows[i + top] = offset - (left >> 2);
			break;
		case 3:
			view->rows[i + top] = offset - (left >> 1);
			break;
		default:
			view->rows[i + top] = offset - left;
			break;
		}
		offset += rowBytes;
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
