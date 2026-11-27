/* The player's settings, kept in <data>/settings.json as whole numbers:
 *
 *   {"aspect": 0, "npc_pace": 1, "runes": 1, ...}
 *
 * A switch on the command line changes a setting for that run only. Names in the file that
 * this program doesn't know are kept when it is written back.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "backend.h"

static const struct {
	const char *name;
	int16_t value;
} defaults[] = {
	{"aspect", 0},          /* 0 4:3, 1 16:10, 2 16:9 */
	{"npc_pace", 1},        /* 1 townsfolk at half speed, 0 as the originals */
	{"runes", 1},           /* 1 signs translated, 0 in their runes */
	{"fullscreen", 0},
	{"frame_limit", PLAT_GAME_FPS_MAX},
	{"volume", 10},         /* tenths of full */
	{"sound", 0},           /* 0 best available, 1 Adlib, 2 MT32, 3 OPL3 */
	{"filter", 0},          /* 0 fill the window, 1 pixel-exact */
	{"cheats", 0},          /* the cheat keys */
};
#define SETTING_COUNT (sizeof defaults / sizeof *defaults)

static char data_dir[1024] = ".";
static MTY_JSON *saved;
static bool overridden[SETTING_COUNT];
static int16_t overrides[SETTING_COUNT];

static int find(const char *name)
{
	for (size_t i = 0; i < SETTING_COUNT; i++) {
		if (strcmp(defaults[i].name, name) == 0)
			return (int) i;
	}
	return -1;
}

const char *settings_data_dir(void)
{
	return data_dir;
}

/* The local folder when given, else the OS's folder for a user's program data; U7_USER_DATA
 * overrides both. */
static void find_data_dir(const char *local)
{
	const char *dir = getenv("U7_USER_DATA");

	if (dir != NULL && dir[0] != '\0') {
		snprintf(data_dir, sizeof data_dir, "%s", dir);
		return;
	}
	if (local != NULL) {
		snprintf(data_dir, sizeof data_dir, "%s", local);
		return;
	}
#if defined(_WIN32)
	snprintf(data_dir, sizeof data_dir, "%s\\Ultima7Modern", MTY_GetDir(MTY_DIR_HOME));
#elif defined(__APPLE__)
	snprintf(data_dir, sizeof data_dir, "%s/Library/Application Support/Ultima7Modern",
		MTY_GetDir(MTY_DIR_HOME));
#else
	dir = getenv("XDG_DATA_HOME");
	if (dir != NULL && dir[0] != '\0')
		snprintf(data_dir, sizeof data_dir, "%s/ultima7modern", dir);
	else
		snprintf(data_dir, sizeof data_dir, "%s/.local/share/ultima7modern", MTY_GetDir(MTY_DIR_HOME));
#endif
}

void settings_load(const char *local)
{
	find_data_dir(local);
	saved = MTY_JSONReadFile(MTY_JoinPath(data_dir, "settings.json"));
	if (saved != NULL && MTY_JSONGetType(saved) != MTY_JSON_OBJECT)
		MTY_JSONDestroy(&saved);
	if (saved == NULL)
		saved = MTY_JSONObjCreate();
}

void settings_override(const char *name, int16_t value)
{
	int i = find(name);

	if (i >= 0) {
		overridden[i] = true;
		overrides[i] = value;
	}
}

int16_t plat_setting(const char *name)
{
	int i = find(name);
	int16_t value;

	/* keys only tests write, such as no_start_items, read from the file alone */
	if (i < 0)
		return MTY_JSONObjGetInt16(saved, name, &value) ? value : 0;
	if (overridden[i])
		value = overrides[i];
	else if (!MTY_JSONObjGetInt16(saved, name, &value))
		value = defaults[i].value;
	if (strcmp(name, "frame_limit") == 0 && (value < PLAT_GAME_FPS_MIN || value > PLAT_GAME_FPS_MAX))
		value = PLAT_GAME_FPS_MAX;
	return value;
}

void plat_set_setting(const char *name, int16_t value)
{
	int i = find(name);

	if (i >= 0) {
		overridden[i] = false;
		MTY_JSONObjSetInt(saved, name, value);
	}
	if (strcmp(name, "volume") == 0)
		audio_set_volume(value);
}

void plat_save_settings(void)
{
	int16_t value;

	/* Every setting goes in the file, so it lists what can be changed. */
	for (size_t i = 0; i < SETTING_COUNT; i++) {
		if (!MTY_JSONObjGetInt16(saved, defaults[i].name, &value))
			MTY_JSONObjSetInt(saved, defaults[i].name, defaults[i].value);
	}
	if (!files_make_dir(data_dir) ||!MTY_JSONWriteFile(MTY_JoinPath(data_dir, "settings.json"), saved))
		fprintf(stderr, "u7: could not write the settings to %s\n", data_dir);
}
