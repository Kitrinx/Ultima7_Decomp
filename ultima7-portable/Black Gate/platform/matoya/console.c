/* Text the DOS game wrote with printf and cprintf in its 320x200 graphics mode. The BIOS drew
 * each character from its 8x8 font straight into screen memory, colour 7 on colour 0, at a
 * 40 by 25 text cursor; it wrapped at the right edge and scrolled the whole screen up a text row
 * at the bottom. The text stays until the game draws over it.
 */
#include <string.h>

#include "backend.h"
#include "vgafont8.h"

#define COLUMNS 40
#define ROWS 25
#define GLYPH 8
#define WIDTH PLAT_SCREEN_WIDTH
#define TEXT_COLOUR 7
#define BACK_COLOUR 0

/* Game thread. */
static int16_t column, row;

void console_reset(void)
{
	column = 0;
	row = 0;
}

void plat_console_goto(int16_t x, int16_t y)
{
	column = x < 1 ? 0 : x > COLUMNS ? COLUMNS - 1 : x - 1;
	row = y < 1 ? 0 : y > ROWS ? ROWS - 1 : y - 1;
}

static void draw_char(uint8_t *screen, uint8_t ch)
{
	uint8_t *at = screen + (row * GLYPH) * WIDTH + column * GLYPH;

	for (int y = 0; y < GLYPH; y++, at += WIDTH) {
		for (int x = 0; x < GLYPH; x++)
			at[x] = VgaFont8[ch][y] & (0x80 >> x) ? TEXT_COLOUR : BACK_COLOUR;
	}
}

static void next_row(uint8_t *screen)
{
	if (++row < ROWS)
		return;
	row = ROWS - 1;
	memmove(screen, screen + GLYPH * WIDTH, (ROWS - 1) * GLYPH * WIDTH);
	memset(screen + (ROWS - 1) * GLYPH * WIDTH, BACK_COLOUR, GLYPH * WIDTH);
}

void plat_console_write(const char *text)
{
	uint8_t *screen = video_screen();

	if (screen == NULL)
		return;
	for (; *text; text++) {
		switch (*text) {
		case '\r':
			column = 0;
			break;
		case '\n':
			next_row(screen);
			break;
		case '\b':
			if (column > 0)
				column--;
			break;
		case '\a':
			break;
		default:
			draw_char(screen, (uint8_t) *text);
			if (++column == COLUMNS) {
				column = 0;
				next_row(screen);
			}
		}
	}
	video_refresh(true);
}
