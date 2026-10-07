/* Serpent Isle SI.EXE, overlay segment 237 (file offsets 0x067030 to 0x0674d5, 1189 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

/* path: savegame.c */
#include "u7port.h"
#include <string.h>
#include "plat.h"
#include "dosio.h"
#include "easyfile.h"
#include "flex.h"
#include "datanode.h"
#include "u7sound.h"
#include "colbuf.h"
#include "debug.h"
#include "oops.h"
#include "itemovr1.h"
#include "itemovr2.h"
#include "loadreg.h"
#include "sysusage.h"
#include "u7manage.h"
#include "u7npc.h"
#include "savegame.h"
#include "worldpal.h"
#include "text.h"

/* The version stamped in a saved game's header. */
#define SAVE_VERSION    300

/* A saved game: a Flex file of the files in the game directory. */
GameFiles SaveGameFiles;

uint8_t SaveLoadActive = 0;
char *const InitGameFileName = "INITGAME.DAT";
char *const SaveGameFileName = "SAVEGAME.DAT";
char *const SaveFileNameFormat = "GAME%02d.U7";
char *const NPCFileName = "NPC.DAT";

/* Free bytes on the current drive. */
int32_t GetDiskFreeBytes(void)
{
	uint32_t bytes = plat_disk_free();

	/* DOS never reported more than 2 GB free. */
	return bytes > INT32_MAX ? INT32_MAX : (int32_t) bytes;
}

/* Free space on the current drive, in units of 400 KB. */
int16_t GameFiles::getFreeSaveSpace()
{
	return GetDiskFreeBytes() / (400 * INT32_C(1024));
}

/* Saves every registered object into dir, through refresh when flag is set. */
void GameFiles::saveAll(char *dir, int8_t flag)
{
	int16_t unusedMode = 0x9200;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		if (flag)
			p->refresh(dir);
		else
			p->save(dir);
		p = p->next;
	}
	SaveLoadActive = 0;
}

/* Loads every registered object from dir. */
void GameFiles::loadAll(char *dir)
{
	int16_t unusedMode = 0x9100;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		p->load(dir);
		p = p->next;
	}
	SaveLoadActive = 0;
}

/* Starts a new game: unpacks INITGAME.DAT into the game directory and loads it. */
int8_t MakeNewGame(void)
{
	SaveGameFile save;

	LogMemoryUsage(GetGameText(3, 217));
	plat_dir_create(GamedatPath);
	save.extract(BuildPath(StaticPath, InitGameFileName, 0), GamedatPath);
	LogMemoryUsage(GetGameText(3, 218));
	LoadNpcs(BuildPath(GamedatPath, NPCFileName, 0));
	plat_file_remove(BuildPath(GamedatPath, NPCFileName, 0));
	LogMemoryUsage(GetGameText(3, 219));
	ForgetSavedRegions();
	LoadGameArgs();
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	RestoreAvatarMana();
	LoadShapesInUse();
	return 1;
}

/* Loads the game from the game directory. */
void GameFiles::loadGame()
{
	loadAll(GamedatPath);
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	ReloadRegionTerrain();
	GameScreen.restore();
}

/* Whether the game directory exists. */
uint8_t GameFiles::hasGameDirectory()
{
	uint8_t exists = 0;

	exists = plat_dir_exists(GamedatPath) != 0;
	return exists;
}

/* The file name of saved game n. */
char *GameFiles::getSaveFileName(int16_t n)
{
	return BuildNumberedPath("", SaveFileNameFormat, n, 0);
}

/* The title of saved game n, or "" when there is none. */
void GameFiles::getSaveTitle(int16_t n, char *title)
{
	Flex f;

	if (!f.open(getSaveFileName(n)))
		strcpy(title, "");
	else {
		f.hdr.getTitle(title);
		if (f.hdr.saveVersion != SAVE_VERSION)
			strcpy(title, "");
		f.close();
	}
}

/* Restores saved game n (-1: the game directory as it stands), starting a new game when there is
 * no game directory. */
void GameFiles::restoreGame(int16_t n)
{
	StopMusic();
	if (n != -1) {
		SaveGameFile save;
		save.extract(getSaveFileName(n), GamedatPath);
	}
	if (!hasGameDirectory()) {
		MakeNewGame();
		saveGame(-1, 0, 1);
	}
	loadGame();
	PlayMusic(CurrentMusic);
}

/* Saves the game into the game directory and, unless n is -1, packs it as saved game n under
 * title. */
void GameFiles::saveGame(int16_t n, char *title, int8_t flag)
{
	saveAll(GamedatPath, flag);
	if (n != -1) {
		SaveGameFile save;
		save.save(GamedatPath, getSaveFileName(n), title, SAVE_VERSION, 300);
	}
}

/* Deletes the game directory and the files in it. */
void GameFiles::deleteGameDirectory()
{
	uint8_t done;
	plat_find ff;

	if (hasGameDirectory()) {
		done = !plat_find_first(BuildPath(GamedatPath, "*.*", 0), &ff);
		while (!done) {
			plat_file_remove(BuildPath(GamedatPath, ff.name, 0));
			done = !plat_find_next(&ff);
		}
		plat_dir_remove(GamedatPath);
	}
}

extern "C" void ResetSavegameGlobals(void)
{
	memset(&SaveGameFiles, 0, sizeof(SaveGameFiles));
	SaveLoadActive = 0;
}
