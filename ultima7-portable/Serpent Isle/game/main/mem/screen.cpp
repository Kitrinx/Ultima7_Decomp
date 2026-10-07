/* Serpent Isle SI.EXE, resident segment 143 (file offsets 0x03ecc6 to 0x03ee7c, 438 bytes).
 * Borland C++ 2.0 -mm -O -G -P -vi- rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#pragma inline
#include "u7port.h"
#include "plat.h"
#include "arena.h"
#include "lowlevel.h"
#include "dosio.h"
#include "view.h"
#include "memapi.h"
#include "screen.h"

View *VgaScreen;
View *BackScreen;
int32_t VgaRowTable;
int16_t UnreadScreenWord;
View *ActiveScreen;
int16_t UnusedScreenWord;
int16_t *UnreadScreenWordPointer;

inline void SetRect(Rect *r, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	r->set(x0, y0, x1, y1);
}

extern "C" int16_t InitVgaScreen(View *view, uint8_t color)
{
	uint32_t row;
	int32_t address;

	VgaScreen = view;
	UnreadScreenWordPointer = &UnreadScreenWord;
	UnreadScreenWord = 0;
	ActiveScreen = view;
	view->clip.set(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	view->setSegment(0);
	VgaRowTable = PointerToLinear(AllocateFarHeap(SCREEN_HEIGHT * sizeof(int32_t), 0));
	view->rowTable.set(VgaRowTable);
	for (row = 0, address = PointerToLinear(ScreenPixels()); row < SCREEN_HEIGHT; row++, address += SCREEN_WIDTH)
		view->rowTable.setRow((uint16_t)row, address);
	if (color != -1)
		FillView(view, color);
	/* VGA memory was always on screen: from now on the host shows this buffer as drawn. */
	plat_video_present(ScreenPixels());
	return 1;
}

int16_t InitBackScreen(View *view, uint8_t color)
{
	BackScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	return AllocateDrawBuffer(view, color, DRAW_IN_XMS);
}

extern "C" void ResetScreenGlobals(void)
{
	VgaScreen = 0;
	BackScreen = 0;
	VgaRowTable = 0;
	UnreadScreenWord = 0;
	ActiveScreen = 0;
	UnusedScreenWord = 0;
	UnreadScreenWordPointer = 0;
}
