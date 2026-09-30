/* Black Gate U7.EXE, resident segment 144 (file offsets 0x03f268 to 0x03f37b, 275 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

/* Built through the assembler, which emits the fixups in address order. */
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
int16_t UnreadScreenWord;           /* zeroed, and pointed at below; nothing reads either */
View *ActiveScreen;
int16_t UnusedScreenWord;
int16_t *UnreadScreenWordPointer;

inline void SetRect(Rect *r, int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	r->x0 = x0;
	r->y0 = y0;
	r->x1 = x1;
	r->y1 = y1;
}

/* Make view the VGA screen, its rows the lines of the screen buffer, and fill it with color. */
extern "C" int16_t InitVgaScreen(View *view, uint8_t color)
{
	uint32_t row;
	int32_t address;

	VgaScreen = view;
	UnreadScreenWordPointer = &UnreadScreenWord;
	UnreadScreenWord = 0;
	ActiveScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	view->segment = 0;
	VgaRowTable = PointerToLinear(AllocateFarHeap(SCREEN_HEIGHT * sizeof(int32_t), 0));
	view->rowTable = VgaRowTable;
	for (row = 0, address = PointerToLinear(ScreenPixels()); row < SCREEN_HEIGHT; row++, address += SCREEN_WIDTH)
		SetRowAddress((uint16_t) row, address, view->rowTable);
	if (color != -1)                /* always true: color is unsigned */
		FillView(view, color);
	/* VGA memory was always on screen: from now on the host shows this buffer as drawn. */
	plat_video_present(ScreenPixels());
	return 1;
}

/* Make view the back screen, with a buffer of its own in XMS. */
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
