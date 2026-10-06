/* Black Gate U7.EXE, resident segment 144 (file offsets 0x03f268 to 0x03f37b, 275 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

/* Built through the assembler, which emits the fixups in address order. */
#pragma inline

#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "memapi.h"
#include "screen.h"

View *VgaScreen;
View *BackScreen;
long VgaRowTable;
int UnreadScreenWord;           /* zeroed, and pointed at below; nothing reads either */
View *ActiveScreen;
int UnusedScreenWord;
int *UnreadScreenWordPointer;

inline void SetRect(Rect *r, int x0, int y0, int x1, int y1)
{
	r->x0 = x0;
	r->y0 = y0;
	r->x1 = x1;
	r->y1 = y1;
}

/* Make view the VGA screen, its rows the lines of video memory, and fill it with color. */
extern "C" int far pascal InitVgaScreen(View *view, unsigned char color)
{
	unsigned long row;
	long address;

	VgaScreen = view;
	UnreadScreenWordPointer = &UnreadScreenWord;
	UnreadScreenWord = 0;
	ActiveScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	view->segment = 0;
	VgaRowTable = PointerToLinear(AllocateFarHeap(SCREEN_HEIGHT * sizeof(long), 0));
	view->rowTable = VgaRowTable;
	for (row = 0, address = 0xa0000L; row < SCREEN_HEIGHT; row++, address += SCREEN_WIDTH)
		SetRowAddress((unsigned) row, address, view->rowTable);
	if (color != -1)                /* always true: color is unsigned */
		FillView(view, color);
	return 1;
}

/* Make view the back screen, with a buffer of its own in XMS. */
int far pascal InitBackScreen(View *view, unsigned char color)
{
	BackScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	return AllocateDrawBuffer(view, color, DRAW_IN_XMS);
}
