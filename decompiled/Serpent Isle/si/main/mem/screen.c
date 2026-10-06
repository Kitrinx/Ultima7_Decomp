/* Serpent Isle SI.EXE, resident segment 143 (file offsets 0x03ecc6 to 0x03ee7c, 438 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#pragma inline
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "memapi.h"
#include "screen.h"

View *VgaScreen;
View *BackScreen;
long VgaRowTable;
int UnreadScreenWord;
View *ActiveScreen;
int UnusedScreenWord;
int *UnreadScreenWordPointer;

inline void SetRect(Rect *r, int x0, int y0, int x1, int y1)
{
	r->set(x0, y0, x1, y1);
}

extern "C" int far pascal InitVgaScreen(View *view, unsigned char color)
{
	unsigned long row;
	long address;

	VgaScreen = view;
	UnreadScreenWordPointer = &UnreadScreenWord;
	UnreadScreenWord = 0;
	ActiveScreen = view;
	view->clip.set(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	view->setSegment(0);
	VgaRowTable = PointerToLinear(AllocateFarHeap(SCREEN_HEIGHT * sizeof(long), 0));
	view->rowTable.set(VgaRowTable);
	for (row = 0, address = 0xa0000L; row < SCREEN_HEIGHT; row++, address += SCREEN_WIDTH)
		view->rowTable.setRow((unsigned)row, address);
	if (color != -1)
		FillView(view, color);
	return 1;
}

int far pascal InitBackScreen(View *view, unsigned char color)
{
	BackScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	return AllocateDrawBuffer(view, color, DRAW_IN_XMS);
}
