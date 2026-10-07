/* Text the DOS game printed: printf went through DOS to the BIOS teletype, cprintf through
 * Borland's conio, which kept its own window and text attribute and wrote each character with
 * the BIOS (directvideo = 0). Both share the BIOS cursor.
 *
 * Graphics mode (13h): the BIOS drew 8x8 characters from its ROM font into screen memory at a
 * 40 by 25 cursor, on black. The teletype drew in colour 7, wrapped at 40 and scrolled the whole
 * screen; conio drew in its attribute and wrapped and scrolled inside its window.
 *
 * Text mode (textmode(C80)): the screen is 80 by 25 cells of character and attribute, as in
 * video memory at B800. The teletype keeps a cell's attribute; conio sets it. video.c draws the
 * cells with the VGA 9x16 font.
 *
 * conio learns the screen size only at start-up and in textmode, so after a return to mode 13h
 * by the BIOS its window is still 80 wide; columns past 40 then land further along screen memory.
 */
#include <string.h>

#include "backend.h"
#include "vgafont8.h"

#define ROWS 25
#define GRAPHICS_COLUMNS 40
#define TEXT_COLUMNS 80
#define GLYPH 8
#define WIDTH PLAT_SCREEN_WIDTH
#define SCREEN_BYTES (PLAT_SCREEN_WIDTH * PLAT_SCREEN_HEIGHT)
#define TELETYPE_COLOUR 7
#define BLANK_CELL 0x0720

/* Game thread. */
static bool text_mode;
static int16_t cursor_x, cursor_y;
static uint16_t cells[ROWS * TEXT_COLUMNS];
/* conio's state, from 0: the screen width it knows, its window and attribute. */
static int16_t screen_columns;
static int16_t window_left, window_top, window_right, window_bottom;
static uint8_t text_attribute;

void console_reset(void)
{
	text_mode = false;
	cursor_x = 0;
	cursor_y = 0;
	screen_columns = GRAPHICS_COLUMNS;
	window_left = 0;
	window_top = 0;
	window_right = GRAPHICS_COLUMNS - 1;
	window_bottom = ROWS - 1;
	/* conio read it from the blank mode 13h screen at start-up */
	text_attribute = 0;
}

static int16_t bios_columns(void)
{
	return text_mode ? TEXT_COLUMNS : GRAPHICS_COLUMNS;
}

static void show(void)
{
	if (text_mode)
		video_show_text(cells, cursor_x, cursor_y);
	else
		video_refresh(true);
}

/* The BIOS's write character and attribute in mode 13h: the glyph in the colour on black, at
 * column * 8 + row * 2560 bytes into screen memory. */
static void draw_glyph(int16_t x, int16_t y, uint8_t ch, uint8_t colour)
{
	uint8_t *screen = video_screen();
	int32_t at = (int32_t) y * GLYPH * WIDTH + x * GLYPH;

	if (screen == NULL)
		return;
	for (int row = 0; row < GLYPH; row++, at += WIDTH) {
		for (int col = 0; col < GLYPH; col++) {
			if (at + col < SCREEN_BYTES)
				screen[at + col] = VgaFont8[ch][row] & (0x80 >> col) ? colour : 0;
		}
	}
}

/* The BIOS's scroll up by one row of a rectangle, the new row filled with the attribute; in
 * mode 13h the attribute is the fill colour. */
static void scroll_up(int16_t left, int16_t top, int16_t right, int16_t bottom, uint8_t attribute,
	bool clear)
{
	if (text_mode) {
		for (int16_t y = top; y <= bottom; y++) {
			uint16_t *row = cells + y * TEXT_COLUMNS;

			if (y < bottom && !clear)
				memcpy(row + left, row + TEXT_COLUMNS + left, (right - left + 1) * sizeof *row);
			else
				for (int16_t x = left; x <= right; x++)
					row[x] = 0x20 | attribute << 8;
		}
		return;
	}
	uint8_t *screen = video_screen();

	if (screen == NULL)
		return;
	if (right >= GRAPHICS_COLUMNS)
		right = GRAPHICS_COLUMNS - 1;
	for (int16_t y = top * GLYPH; y < (bottom + 1) * GLYPH && left <= right; y++) {
		uint8_t *row = screen + y * WIDTH + left * GLYPH;
		size_t width = (size_t) (right - left + 1) * GLYPH;

		if (y < bottom * GLYPH && !clear)
			memcpy(row, row + GLYPH * WIDTH, width);
		else
			memset(row, attribute, width);
	}
}

static void teletype_new_line(void)
{
	if (++cursor_y < ROWS)
		return;
	cursor_y = ROWS - 1;
	/* text mode fills with the attribute under the cursor, graphics with black */
	scroll_up(0, 0, bios_columns() - 1, ROWS - 1,
		text_mode ? cells[cursor_y * TEXT_COLUMNS + cursor_x] >> 8 : 0, false);
}

void plat_console_write(const char *text)
{
	for (; *text; text++) {
		uint8_t ch = (uint8_t) *text;

		switch (ch) {
		case '\r':
			cursor_x = 0;
			break;
		case '\n':
			teletype_new_line();
			break;
		case '\b':
			if (cursor_x > 0)
				cursor_x--;
			break;
		case '\a':
			break;
		default:
			if (text_mode) {
				uint16_t *cell = &cells[cursor_y * TEXT_COLUMNS + cursor_x];

				*cell = (uint16_t) ((*cell & 0xFF00) | ch);
			} else {
				draw_glyph(cursor_x, cursor_y, ch, TELETYPE_COLOUR);
			}
			if (++cursor_x == bios_columns()) {
				cursor_x = 0;
				teletype_new_line();
			}
		}
	}
	show();
}

static void conio_put(uint8_t ch)
{
	switch (ch) {
	case '\a':
		break;
	case '\b':
		if (cursor_x > window_left)
			cursor_x--;
		break;
	case '\n':
		cursor_y++;
		break;
	case '\r':
		cursor_x = window_left;
		break;
	default:
		if (text_mode)
			cells[cursor_y * TEXT_COLUMNS + cursor_x] = (uint16_t) (ch | text_attribute << 8);
		else
			draw_glyph(cursor_x, cursor_y, ch, text_attribute);
		cursor_x++;
	}
	if (cursor_x > window_right) {
		cursor_x = window_left;
		cursor_y++;
	}
	if (cursor_y > window_bottom) {
		scroll_up(window_left, window_top, window_right, window_bottom, text_attribute, false);
		cursor_y--;
	}
}

void plat_console_cputs(const char *text)
{
	for (; *text; text++)
		conio_put((uint8_t) *text);
	show();
}

void plat_console_putch(uint8_t ch)
{
	conio_put(ch);
	show();
}

void plat_console_goto(int16_t x, int16_t y)
{
	if (x < 1 || y < 1 || window_left + x - 1 > window_right || window_top + y - 1 > window_bottom)
		return;
	cursor_x = window_left + x - 1;
	cursor_y = window_top + y - 1;
	show();
}

int16_t plat_console_where_x(void)
{
	return cursor_x - window_left + 1;
}

int16_t plat_console_where_y(void)
{
	return cursor_y - window_top + 1;
}

void plat_console_window(int16_t left, int16_t top, int16_t right, int16_t bottom)
{
	if (left < 1 || top < 1 || right > screen_columns || bottom > ROWS || left > right || top > bottom)
		return;
	window_left = left - 1;
	window_top = top - 1;
	window_right = right - 1;
	window_bottom = bottom - 1;
	cursor_x = window_left;
	cursor_y = window_top;
	show();
}

void plat_console_text_color(int16_t color)
{
	text_attribute = (uint8_t) ((text_attribute & 0x70) | (color & 0x8F));
}

void plat_console_text_background(int16_t color)
{
	text_attribute = (uint8_t) ((text_attribute & 0x8F) | ((color << 4) & 0x70));
}

void plat_console_clrscr(void)
{
	scroll_up(window_left, window_top, window_right, window_bottom, text_attribute, true);
	cursor_x = window_left;
	cursor_y = window_top;
	show();
}

void plat_console_text_mode(void)
{
	/* conio set the mode only when it differed */
	if (!text_mode) {
		text_mode = true;
		for (int i = 0; i < ROWS * TEXT_COLUMNS; i++)
			cells[i] = BLANK_CELL;
		cursor_x = 0;
		cursor_y = 0;
	}
	screen_columns = TEXT_COLUMNS;
	window_left = 0;
	window_top = 0;
	window_right = TEXT_COLUMNS - 1;
	window_bottom = ROWS - 1;
	/* its start-up attribute again */
	text_attribute = 0;
	show();
}

void plat_console_graphics_mode(void)
{
	text_mode = false;
	cursor_x = 0;
	cursor_y = 0;
	video_show_game();
}
