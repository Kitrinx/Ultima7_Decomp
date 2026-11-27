/* The start menu: pick Black Gate or Serpent Isle, change settings, or quit.
 *
 *        [ highlighted game's logo ]
 *
 *        red gate          blue gate
 *       Black Gate       Serpent Isle
 *     ----------------------------------
 *         Settings           Exit
 *
 * Exit code 1 plays Black Gate, 2 Serpent Isle, 0 quits.
 */
#include <stdlib.h>
#include <string.h>
#include "plat.h"
#include "art.h"
#include "gate.h"

#define EXIT_QUIT 0
#define EXIT_BLACK_GATE 1
#define EXIT_SERPENT_ISLE 2

#define KEY_ENTER 13
#define KEY_ESCAPE 27
#define KEY_UP (256 + 72)
#define KEY_DOWN (256 + 80)
#define KEY_LEFT (256 + 75)
#define KEY_RIGHT (256 + 77)

/* MAINSHP.FLX entries, the same in both games */
#define SHAPE_LOGO 2
#define SHAPE_FONT 9
#define SHAPE_POINTER 19
#define SI_MENU_PALETTE 26
#define BG_MENU_PALETTE 6       /* in INTROPAL.DAT */

/* The gates take Black Gate's moongate reds, 18 (lightest) to 25, and its blue gate's
 * matching blues 75-82. */
#define GATE_RED_FIRST 18
#define GATE_RED_LAST 25
#define GATE_BLUE_SHIFT 57
#define GATE_SIDE_RED 22

#define FULL 63
#define FADE_TICKS 20
#define GATE_FRAME_TICKS 6

#define LOGO_TOP 8
#define LOGO_HEIGHT 56
#define GATE_BOTTOM 132
#define GATE_SPREAD 64
#define LABEL_Y 146
#define DIVIDER_Y 160
#define DIVIDER_HALF 96
#define BOTTOM_ROW_Y 180

/* The rows of each logo picture holding the title alone, without the subtitles below it */
static const struct {
	int16_t first, last;
} LogoTitleRows[2] = {{14, 65}, {0, 34}};

enum { ITEM_BLACK_GATE, ITEM_SERPENT_ISLE, ITEM_SETTINGS, ITEM_EXIT, ITEM_COUNT };

typedef struct {
	int16_t left, top, right, bottom;
} Box;

typedef struct {
	ArtScreen *screen;
	ArtEntry logo[2];
	uint8_t logoMap[2][256];
	int16_t logoFirst[2], logoCount[2];
	uint8_t (*gateArt)[GATE_HEIGHT][GATE_WIDTH];      /* each frame */
	uint8_t gateMap[2][2][GATE_SHADES + 1];         /* [game][highlighted] */
	ArtEntry font;
	uint8_t textMap[256], litMap[256];
	uint8_t dividerColour;
	ArtEntry pointer;
	uint8_t pointerMap[256];
	int16_t mouseX, mouseY;
	uint16_t buttons;
	uint32_t tick;
} Menu;

/* ---- Setting up ---- */

static void Dim(ArtPalette palette, int16_t first, int16_t last)
{
	int16_t i;

	for (i = 3 * first; i < 3 * (last + 1); ++i)
		palette[i] = (uint8_t) (palette[i] / 2);
}

/* Lighter, for the highlighted text. */
static void Lighten(ArtPalette palette)
{
	int16_t i;

	for (i = 0; i < 768; ++i)
		palette[i] = (uint8_t) (palette[i] + (FULL - palette[i]) * 3 / 4);
}

static uint8_t AddColour(ArtScreen *screen, uint8_t red, uint8_t green, uint8_t blue)
{
	uint8_t *rgb = screen->rgb + 3 * screen->used;

	rgb[0] = red;
	rgb[1] = green;
	rgb[2] = blue;
	return (uint8_t) screen->used++;
}

static int16_t LoadPalette(int16_t game, ArtPalette palette)
{
	ArtEntry entry = game == GAME_SI
		? ArtLoad(GAME_SI, "STATIC\\MAINSHP.FLX", SI_MENU_PALETTE)
		: ArtLoad(GAME_BG, "STATIC\\INTROPAL.DAT", BG_MENU_PALETTE);
	int16_t ok = ArtPaletteFrom(&entry, palette);

	ArtFree(&entry);
	return ok;
}

static void LoadLogo(Menu *menu, int16_t game)
{
	ArtPalette palette;

	menu->logo[game] = ArtLoad(game, "STATIC\\MAINSHP.FLX", SHAPE_LOGO);
	if (!menu->logo[game].data || !LoadPalette(game, palette)) {
		ArtFree(&menu->logo[game]);
		return;
	}
	menu->logoFirst[game] = menu->screen->used;
	ArtAddColours(menu->screen, &menu->logo[game], palette, menu->logoMap[game]);
	menu->logoCount[game] = (int16_t) (menu->screen->used - menu->logoFirst[game]);
}

/* A slot for each shade of one colouring of the gate; its side takes the shade of red 22. */
static void AddGateColours(ArtScreen *screen, const ArtPalette palette, uint8_t *map)
{
	int16_t shade;

	for (shade = 0; shade < GATE_SHADES; ++shade) {
		const uint8_t *rgb = palette + 3 * (GATE_RED_LAST - shade);

		map[shade] = AddColour(screen, rgb[0], rgb[1], rgb[2]);
	}
	map[GATE_SIDE] = map[GATE_RED_LAST - GATE_SIDE_RED];
}

static void LoadGates(Menu *menu)
{
	ArtEntry entry = ArtLoad(GAME_BG, "STATIC\\PALETTES.FLX", 0);
	ArtPalette red, blue;
	int16_t i, found = ArtPaletteFrom(&entry, red);

	ArtFree(&entry);
	if (!found)
		return;
	menu->gateArt = (uint8_t (*)[GATE_HEIGHT][GATE_WIDTH])
		malloc(GATE_FRAMES * sizeof *menu->gateArt);
	if (!menu->gateArt)
		return;
	for (i = 0; i < GATE_FRAMES; ++i)
		GateDrawFrame(menu->gateArt[i], i);
	memcpy(blue, red, sizeof blue);
	for (i = GATE_RED_FIRST; i <= GATE_RED_LAST; ++i)
		memcpy(blue + 3 * i, red + 3 * (i + GATE_BLUE_SHIFT), 3);
	AddGateColours(menu->screen, red, menu->gateMap[GAME_BG][1]);
	AddGateColours(menu->screen, blue, menu->gateMap[GAME_SI][1]);
	Dim(red, GATE_RED_FIRST, GATE_RED_LAST);
	Dim(blue, GATE_RED_FIRST, GATE_RED_LAST);
	AddGateColours(menu->screen, red, menu->gateMap[GAME_BG][0]);
	AddGateColours(menu->screen, blue, menu->gateMap[GAME_SI][0]);
}

/* Black Gate's menu buttons light up one shade: 132 to 131, 133 and 134 to 132, 135 and 136 to 133. */
static void BgHighlight(Menu *menu, const ArtPalette palette)
{
	static const uint8_t lit[5] = {131, 132, 132, 133, 133};
	int16_t i;

	memcpy(menu->litMap, menu->textMap, 256);
	for (i = 0; i < 5; ++i) {
		const uint8_t *rgb = palette + 3 * lit[i];

		menu->litMap[132 + i] = AddColour(menu->screen, rgb[0], rgb[1], rgb[2]);
	}
}

/* The menu font and pointer, from Black Gate's menu when it is there, else Serpent Isle's. */
static void LoadFont(Menu *menu)
{
	ArtPalette palette;
	int16_t game;

	for (game = GAME_BG; game <= GAME_SI; ++game) {
		menu->font = ArtLoad(game, "STATIC\\MAINSHP.FLX", SHAPE_FONT);
		if (menu->font.data && LoadPalette(game, palette))
			break;
		ArtFree(&menu->font);
	}
	if (!menu->font.data)
		return;
	menu->pointer = ArtLoad(game, "STATIC\\MAINSHP.FLX", SHAPE_POINTER);
	ArtAddColours(menu->screen, &menu->pointer, palette, menu->pointerMap);
	ArtAddColours(menu->screen, &menu->font, palette, menu->textMap);
	if (game == GAME_BG) {
		BgHighlight(menu, palette);
		return;
	}
	Lighten(palette);
	ArtAddColours(menu->screen, &menu->font, palette, menu->litMap);
}

static void FreeMenu(Menu *menu)
{
	ArtFree(&menu->logo[GAME_BG]);
	ArtFree(&menu->logo[GAME_SI]);
	free(menu->gateArt);
	ArtFree(&menu->font);
	ArtFree(&menu->pointer);
	free(menu->screen);
}

/* ---- Each frame ---- */

/* Shows the palette at level (0 black to FULL), with the logo colours of game at logoLevel. */
static void ShowPalette(Menu *menu, int16_t level, int16_t game, int16_t logoLevel)
{
	uint8_t rgb[256 * 3];
	int16_t i;

	for (i = 0; i < 768; ++i)
		rgb[i] = (uint8_t) (menu->screen->rgb[i] * level / FULL);
	if (game >= 0) {
		for (i = 3 * menu->logoFirst[game]; i < 3 * (menu->logoFirst[game] + menu->logoCount[game]); ++i)
			rgb[i] = (uint8_t) (rgb[i] * logoLevel / FULL);
	}
	plat_video_set_palette(0, 256, rgb);
}

/* Waits for the next refresh and reads the input: the key, or 0, and the mouse. */
static int16_t NextFrame(Menu *menu)
{
	int16_t key = 0;

	plat_video_wait_retrace();
	menu->tick = plat_ticks();
	plat_mouse_state(&menu->mouseX, &menu->mouseY, &menu->buttons);
	menu->mouseX = (int16_t) (menu->mouseX / 2);
	if (plat_key_available()) {
		key = plat_key_get();
		if (key == 0)
			key = (int16_t) (256 + plat_key_get());
	}
	return key;
}

static void Clear(Menu *menu)
{
	memset(menu->screen->pixels, 0, sizeof menu->screen->pixels);
}

static void Present(Menu *menu)
{
	ArtDraw(menu->screen, &menu->pointer, 0, menu->mouseX, menu->mouseY, menu->pointerMap);
	plat_video_present(menu->screen->pixels);
}

static int16_t Inside(const Box *box, int16_t x, int16_t y)
{
	return x >= box->left && x <= box->right && y >= box->top && y <= box->bottom;
}

/* Text centred on x with its baseline at y; returns where it lies. */
static Box CentredText(Menu *menu, int16_t x, int16_t y, const char *text, int16_t lit)
{
	Box box;
	int16_t width = ArtTextWidth(&menu->font, text);

	box.left = (int16_t) (x - width / 2);
	box.right = (int16_t) (box.left + width);
	box.top = (int16_t) (y - 9);
	box.bottom = (int16_t) (y + 2);
	ArtText(menu->screen, &menu->font, lit ? menu->litMap : menu->textMap, box.left, y, text);
	return box;
}

/* ---- Settings ---- */

#define SETTING_TOP 48
#define SETTING_ROW 12
#define SETTING_LABEL_X (-128)
#define SETTING_VALUE_X 40

static const struct {
	const char *name;
	const char *label;
	int16_t first;
	int16_t count;
	const char *values[11];
} Settings[] = {
	{"aspect", "Screen shape", 0, 3, {"4:3", "16:10", "16:9"}},
	{"filter", "Scaling", 0, 2, {"Fill", "Pixel-exact"}},
	{"fullscreen", "Full screen", 0, 2, {"Off", "On"}},
	{"frame_limit", "Frame limit", PLAT_GAME_FPS_MIN, PLAT_GAME_FPS_MAX - PLAT_GAME_FPS_MIN + 1,
		{"2 FPS", "3 FPS", "4 FPS", "5 FPS", "6 FPS", "7 FPS", "8 FPS", "9 FPS", "10 FPS"}},
	{"sound", "Sound", 0, 4, {"Best available", "Adlib", "MT32", "OPL3"}},
	{"volume", "Volume", 0, 11, {"0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%"}},
	{"npc_pace", "Townsfolk pace", 0, 2, {"Original", "Relaxed"}},
	{"runes", "Runic writing", 0, 2, {"Original", "Translated"}},
	{"cheats", "Cheats", 0, 2, {"Off", "On"}},
};

#define SETTING_COUNT ((int16_t) (sizeof Settings / sizeof Settings[0]))
#define SETTING_DONE SETTING_COUNT

static int16_t SettingAvailable(int16_t row, int16_t value)
{
	if (strcmp(Settings[row].name, "sound") == 0 && value == 2)
		return plat_mt32_available();
	return 1;
}

static int16_t SettingValue(int16_t row)
{
	int16_t value = (int16_t) (plat_setting(Settings[row].name) - Settings[row].first);

	return value >= 0 && value < Settings[row].count && SettingAvailable(row, value) ? value : 0;
}

static void StepSetting(int16_t row, int16_t step)
{
	int16_t count = Settings[row].count, value = SettingValue(row);

	do {
		value = (int16_t) ((value + step + count) % count);
	} while (!SettingAvailable(row, value));
	plat_set_setting(Settings[row].name, (int16_t) (value + Settings[row].first));
}

static void DrawSettings(Menu *menu, int16_t selected, Box *rows)
{
	int16_t centre = (int16_t) (menu->screen->width / 2);
	int16_t row;

	Clear(menu);
	CentredText(menu, centre, 28, "Settings", 1);
	for (row = 0; row < SETTING_COUNT; ++row) {
		int16_t y = (int16_t) (SETTING_TOP + SETTING_ROW * row);
		const uint8_t *map = row == selected ? menu->litMap : menu->textMap;

		ArtText(menu->screen, &menu->font, map, (int16_t) (centre + SETTING_LABEL_X), y,
			Settings[row].label);
		ArtText(menu->screen, &menu->font, map, (int16_t) (centre + SETTING_VALUE_X), y,
			Settings[row].values[SettingValue(row)]);
		rows[row].left = (int16_t) (centre + SETTING_LABEL_X);
		rows[row].right = (int16_t) (centre - SETTING_LABEL_X);
		rows[row].top = (int16_t) (y - 9);
		rows[row].bottom = (int16_t) (y + 2);
	}
	rows[SETTING_DONE] = CentredText(menu, centre, SETTING_TOP + SETTING_ROW * (SETTING_COUNT + 1),
		"Done", selected == SETTING_DONE);
}

static void RunSettings(Menu *menu)
{
	Box rows[SETTING_COUNT + 1];
	int16_t selected = 0;
	int16_t lastX = menu->mouseX, lastY = menu->mouseY;
	uint16_t lastButtons = menu->buttons;
	int16_t changed = 0;

	DrawSettings(menu, selected, rows);
	for (;;) {
		int16_t key = NextFrame(menu);
		int16_t row, step = 0;

		if (menu->mouseX != lastX || menu->mouseY != lastY) {
			for (row = 0; row <= SETTING_DONE; ++row) {
				if (Inside(&rows[row], menu->mouseX, menu->mouseY))
					selected = row;
			}
		}
		if (menu->buttons & ~lastButtons) {
			for (row = 0; row <= SETTING_DONE; ++row) {
				if (Inside(&rows[row], menu->mouseX, menu->mouseY)) {
					selected = row;
					key = KEY_ENTER;
					if (menu->buttons & ~lastButtons & 2)
						step = -1;
				}
			}
		}
		lastX = menu->mouseX;
		lastY = menu->mouseY;
		lastButtons = menu->buttons;

		if (key == KEY_ESCAPE || (key == KEY_ENTER && selected == SETTING_DONE))
			break;
		if (key == KEY_UP)
			selected = (int16_t) ((selected + SETTING_DONE) % (SETTING_DONE + 1));
		else if (key == KEY_DOWN)
			selected = (int16_t) ((selected + 1) % (SETTING_DONE + 1));
		else if (key == KEY_LEFT)
			step = -1;
		else if (key == KEY_RIGHT || (key == KEY_ENTER && step == 0))
			step = 1;
		if (step != 0 && selected != SETTING_DONE) {
			StepSetting(selected, step);
			changed = 1;
		}
		DrawSettings(menu, selected, rows);
		ShowPalette(menu, FULL, -1, 0);
		Present(menu);
	}
	if (changed)
		plat_save_settings();
}

/* ---- The menu ---- */

static int16_t GateFrame(uint32_t ticks)
{
	uint32_t frame = ticks / GATE_FRAME_TICKS;

	if (frame < GATE_RISE_FRAMES)
		return (int16_t) frame;
	frame = (frame - GATE_RISE_FRAMES) % (GATE_FRAMES - GATE_RISE_FRAMES);
	return (int16_t) (GATE_RISE_FRAMES + frame);
}

static void DrawMenu(Menu *menu, int16_t selected, int16_t logo, uint32_t ticks, Box *items)
{
	static const char *const names[2] = {"Black Gate", "Serpent Isle"};
	ArtScreen *screen = menu->screen;
	int16_t centre = (int16_t) (screen->width / 2);
	int16_t game;

	Clear(menu);
	if (logo >= 0 && menu->logo[logo].data) {
		int16_t first = LogoTitleRows[logo].first, last = LogoTitleRows[logo].last;
		int16_t left, top, right, bottom;

		ArtFrameBounds(&menu->logo[logo], 0, &left, &top, &right, &bottom);
		ArtDrawRows(screen, &menu->logo[logo], 0, (int16_t) (centre - (right - left + 1) / 2 - left),
			(int16_t) (LOGO_TOP + (LOGO_HEIGHT - (last - first + 1)) / 2 - first),
			menu->logoMap[logo], first, last);
	}
	for (game = GAME_BG; game <= GAME_SI; ++game) {
		int16_t x = (int16_t) (centre + (game == GAME_BG ? -GATE_SPREAD : GATE_SPREAD));
		int16_t lit = selected == game;

		items[game] = CentredText(menu, x, LABEL_Y, names[game], lit);
		if (!menu->gateArt)
			continue;
		ArtBlit(screen, &menu->gateArt[GateFrame(ticks)][0][0], GATE_WIDTH, GATE_HEIGHT,
			(int16_t) (x - GATE_WIDTH / 2), (int16_t) (GATE_BOTTOM - GATE_HEIGHT),
			menu->gateMap[game][lit], GATE_EMPTY);
		items[game].top = (int16_t) (GATE_BOTTOM - GATE_HEIGHT);
		items[game].left = (int16_t) (x - GATE_SPREAD / 2);
		items[game].right = (int16_t) (x + GATE_SPREAD / 2);
	}
	memset(screen->pixels + DIVIDER_Y * screen->width + centre - DIVIDER_HALF,
		menu->dividerColour, 2 * DIVIDER_HALF);
	items[ITEM_SETTINGS] = CentredText(menu, (int16_t) (centre - GATE_SPREAD), BOTTOM_ROW_Y,
		"Settings", selected == ITEM_SETTINGS);
	items[ITEM_EXIT] = CentredText(menu, (int16_t) (centre + GATE_SPREAD), BOTTOM_ROW_Y,
		"Exit", selected == ITEM_EXIT);
}

/* Arrow keys: left and right along a row, up and down between the gates and the row below. */
static int16_t MoveSelection(int16_t selected, int16_t key)
{
	int16_t column = (int16_t) (selected % 2);
	int16_t lower = (int16_t) (selected / 2);

	if (key == KEY_LEFT || key == KEY_RIGHT)
		column = (int16_t) (1 - column);
	else if (key == KEY_UP || key == KEY_DOWN)
		lower = (int16_t) (1 - lower);
	return (int16_t) (2 * lower + column);
}

static int16_t RunMenu(Menu *menu)
{
	Box items[ITEM_COUNT];
	uint32_t start = plat_ticks(), last = start, fadeOutAt = 0;
	int16_t selected = ITEM_BLACK_GATE, chosen = -1;
	int16_t logo = GAME_BG, logoLevel = FULL;
	int16_t lastX = -1, lastY = -1;
	uint16_t lastButtons = 0;

	memset(items, 0, sizeof items);
	for (;;) {
		int16_t key = NextFrame(menu);
		int16_t wanted = selected <= ITEM_SERPENT_ISLE ? selected : logo;
		int16_t step = (int16_t) (FULL * (menu->tick - last) / (FADE_TICKS / 2));
		int16_t item, level;

		last = menu->tick;
		/* the shown logo fades out before the other fades in */
		if (wanted != logo) {
			logoLevel = (int16_t) (logoLevel - step);
			if (logoLevel <= 0) {
				logoLevel = 0;
				logo = wanted;
			}
		} else if (logoLevel < FULL) {
			logoLevel = (int16_t) (logoLevel + step < FULL ? logoLevel + step : FULL);
		}

		if (chosen < 0) {
			if (lastX >= 0 && (menu->mouseX != lastX || menu->mouseY != lastY)) {
				for (item = 0; item < ITEM_COUNT; ++item) {
					if (Inside(&items[item], menu->mouseX, menu->mouseY))
						selected = item;
				}
			}
			if (menu->buttons & ~lastButtons & 1) {
				for (item = 0; item < ITEM_COUNT; ++item) {
					if (Inside(&items[item], menu->mouseX, menu->mouseY)) {
						selected = item;
						key = KEY_ENTER;
					}
				}
			}
			if (key == KEY_LEFT || key == KEY_RIGHT || key == KEY_UP || key == KEY_DOWN)
				selected = MoveSelection(selected, key);
			else if (key == KEY_ESCAPE)
				chosen = ITEM_EXIT;
			else if (key == KEY_ENTER && selected == ITEM_SETTINGS)
				RunSettings(menu);
			else if (key == KEY_ENTER)
				chosen = selected;
			if (chosen >= 0)
				fadeOutAt = menu->tick;
		}
		lastX = menu->mouseX;
		lastY = menu->mouseY;
		lastButtons = menu->buttons;

		/* fade in from black at the start, out after a choice */
		if (chosen >= 0)
			level = (int16_t) (FULL - FULL * (int32_t) (menu->tick - fadeOutAt) / FADE_TICKS);
		else
			level = (int16_t) (FULL * (int32_t) (menu->tick - start) / FADE_TICKS);
		level = level < 0 ? 0 : level > FULL ? FULL : level;
		DrawMenu(menu, selected, logo, menu->tick - start, items);
		ShowPalette(menu, level, logo, logoLevel);
		Present(menu);
		if (chosen >= 0 && level == 0)
			return chosen;
	}
}

int16_t StartMenuMain(int16_t argc, char **argv)
{
	Menu menu;
	int16_t chosen;

	(void) argc;
	(void) argv;
	memset(&menu, 0, sizeof menu);
	menu.screen = (ArtScreen *) calloc(1, sizeof *menu.screen);
	if (!menu.screen)
		plat_fatal("Out of memory");
	menu.screen->width = PLAT_SCREEN_WIDTH;
	menu.screen->height = PLAT_SCREEN_HEIGHT;
	menu.screen->used = 1;
	menu.dividerColour = AddColour(menu.screen, 24, 12, 2);
	LoadFont(&menu);
	if (!menu.font.data)
		plat_fatal("The start menu could not read either game's STATIC\\MAINSHP.FLX.");
	LoadGates(&menu);
	LoadLogo(&menu, GAME_BG);
	LoadLogo(&menu, GAME_SI);

	chosen = RunMenu(&menu);
	FreeMenu(&menu);
	if (chosen == ITEM_BLACK_GATE)
		return EXIT_BLACK_GATE;
	if (chosen == ITEM_SERPENT_ISLE)
		return EXIT_SERPENT_ISLE;
	return EXIT_QUIT;
}
