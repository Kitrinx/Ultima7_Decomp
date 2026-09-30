#include "u7port.h"
#include "arena.h"
#include "init.h"
#include "lowlevel.h"
#include "view.h"

/* Screen offset of each row of tiles, eight lines apart. */
const uint16_t TileRowOffsets[] = {
	0, 2560, 5120, 7680, 10240, 12800, 15360, 17920, 20480, 23040,
	25600, 28160, 30720, 33280, 35840, 38400, 40960, 43520, 46080, 48640,
	51200, 53760, 56320, 58880, 61440
};
int32_t ViewportFirstRow = 0;	/* linear address of the viewport's first row */
int16_t FlatModeFlags = 0;

/* Copy an 8x8 tile to the viewport at tile column x and tile row y. */
void DrawTile(int32_t tile, int16_t x, int16_t y)
{
	int32_t dst = TileRowOffsets[(uint16_t)y] + ViewportFirstRow + (uint32_t)(uint16_t)x * 8;
	int16_t i;

	for (i = 0; i < 8; i++) {
		memcpy(LINEAR(dst), LINEAR(tile), 8);
		tile += 8;
		dst += SCREEN_WIDTH;
	}
}

/* Fill a view's clip box with one color, row by row through its row table. */
void FillView(void *view, uint8_t color)
{
	View *v = (View *)view;
	uint32_t left = (uint16_t)v->clip.x0;
	uint32_t width = (uint32_t)(uint16_t)v->clip.x1 - left + 1;
	uint16_t rows = (uint16_t)(v->clip.y1 - v->clip.y0 + 1);
	int32_t rowPtr = v->rowTable + (uint32_t)(uint16_t)v->clip.y0 * 4;

	if (rows == 0)
		return;
	do {
		memset(LINEAR(LinearGet32(rowPtr) + left), color, width);
		rowPtr += 4;
	} while (--rows != 0);
}

extern "C" void ResetDrawtileGlobals(void)
{
	ViewportFirstRow = 0;
	FlatModeFlags = 0;
}
