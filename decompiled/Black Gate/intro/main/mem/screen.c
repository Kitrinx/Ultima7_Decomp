/* Black Gate INTRO.EXE, resident segment 41 (file offsets 0x010f39 to 0x011145, 524 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#pragma inline

#include "lowlevel.h"
#include "view.h"
#include "screen.h"

#define CRTC_INDEX      3d4h
#define CRTC_START_HIGH 0ch

extern "C" {
extern char DisplayMode;
}

View *VgaScreen;
View *BackScreen;
unsigned VgaRowTable[SCREEN_HEIGHT];
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

/* Make view the screen of the current display mode: its segment, the offset of each of its rows
 * in the card's interleaved banks, and a fill with color. */
extern "C" int far pascal InitVgaScreen(View *view, unsigned char color)
{
	int row;

	VgaScreen = view;
	UnreadScreenWordPointer = &UnreadScreenWord;
	UnreadScreenWord = 0;
	ActiveScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	switch (DisplayMode) {
	case 1:                     /* EGA: draw in a video page, and show it */
		if (!AllocateDrawBuffer(view, color, 0))
			return 0;
		row = view->id;
		asm mov     dx, CRTC_INDEX
		asm mov     ax, row
		asm shl     ax, 1
		asm shl     ax, 1
		asm shl     ax, 1
		asm shl     ax, 1
		asm mov     al, CRTC_START_HIGH
		asm out     dx, ax
		break;
	case 2:                     /* CGA: two banks of 80-byte rows */
		view->id = 0xb800;
		view->rows = VgaRowTable;
		VgaRowTable[0] = 0;
		VgaRowTable[1] = 0x2000;
		for (row = 2; row < SCREEN_HEIGHT; row++)
			VgaRowTable[row] = VgaRowTable[row - 2] + 80;
		break;
	case 4:                     /* Hercules: four banks of 90-byte rows, the picture centred */
		view->id = 0xb000;
		view->rows = VgaRowTable;
		VgaRowTable[0] = 0x032f;
		VgaRowTable[1] = 0x232f;
		VgaRowTable[2] = 0x632f;
		VgaRowTable[3] = 0x0389;
		VgaRowTable[4] = 0x4389;
		VgaRowTable[5] = 0x6389;
		VgaRowTable[6] = 0x23e3;
		VgaRowTable[7] = 0x43e3;
		for (row = 8; row < SCREEN_HEIGHT; row++)
			VgaRowTable[row] = VgaRowTable[row - 8] + 270;
		break;
	case 3:                     /* Tandy: four banks of 160-byte rows */
		view->id = 0xb800;
		view->rows = VgaRowTable;
		VgaRowTable[0] = 0;
		VgaRowTable[1] = 0x2000;
		VgaRowTable[2] = 0x4000;
		VgaRowTable[3] = 0x6000;
		for (row = 4; row < SCREEN_HEIGHT; row++)
			VgaRowTable[row] = VgaRowTable[row - 4] + 160;
		break;
	default:
		if (DisplayMode)
			color = 0xff;
		view->id = 0xa000;
		view->rows = VgaRowTable;
		for (row = 0; row < SCREEN_HEIGHT; row++)
			VgaRowTable[row] = row * SCREEN_WIDTH;
		break;
	}
	if (color != -1)                /* always true: color is unsigned */
		FillView(view, color);
	return 1;
}

/* Make view the back screen, with a buffer of its own. */
int far pascal InitBackScreen(View *view, unsigned char color)
{
	BackScreen = view;
	SetRect(&view->clip, 0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1);
	return AllocateDrawBuffer(view, color, 0);
}
