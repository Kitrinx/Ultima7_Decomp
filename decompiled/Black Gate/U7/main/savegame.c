/* Black Gate U7.EXE, overlay segment 254 (file offsets 0x0787f0 to 0x078c9b, 1195 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <dir.h>
#include <dos.h>
#include <io.h>
#include <string.h>
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

/* The version stamped in a saved game's header. */
#define SAVE_VERSION    300

/* A saved game: a Flex file of the files in the game directory. */
struct SaveGameFile : FlexWriter {
	int pending, next;
	int *slots;
	SaveGameFile();
	~SaveGameFile();
	void extract(char *name, char *dir);
	void save(char *dir, char *name, char *title, int version, int count);
};

GameFiles SaveGameFiles;

unsigned char SaveLoadActive = 0;
char *InitGameFileName = "INITGAME.DAT";
char *SaveGameFileName = "SAVEGAME.DAT";
char *SaveFileNameFormat = "GAME%02d.U7";
char *NPCFileName = "NPC.DAT";

/* Free bytes on the current drive. */
long GetDiskFreeBytes(void)
{
	long bytes;
	struct REGPACK r;
	struct BYTEREGS *b = (struct BYTEREGS *) &r;

	b->ah = 0x36;
	b->dl = getdisk() + 1;
	/* AX sectors per cluster (FFFF for a bad drive), BX free clusters, CX bytes per sector */
	intr(0x21, &r);
	if (r.r_ax == 0xFFFFL)
		ReportError(0xa606);
	bytes = r.r_ax;
	bytes *= r.r_cx;
	bytes *= r.r_bx;
	return bytes;
}

/* Free space on the current drive, in units of 400 KB. */
int GameFiles::getFreeSaveSpace()
{
	return GetDiskFreeBytes() / (400 * 1024L);
}

/* Saves every registered object into dir, through refresh when flag is set. */
void GameFiles::saveAll(char *dir, char flag)
{
	int unusedMode = 0x9200;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		DebugPrintf("Saving \"%s\"\n", p->getName());
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
	int unusedMode = 0x9100;
	DataNode *p = SaveNodes;

	SaveLoadActive = 1;
	while (p != 0) {
		DebugPrintf("Loading \"%s\"\n", p->getName());
		p->load(dir);
		p = p->next;
	}
	SaveLoadActive = 0;
}

/* Starts a new game: unpacks INITGAME.DAT into the game directory and loads it. */
char MakeNewGame(void)
{
	SaveGameFile save;

	LogMemoryUsage("Making new game");
	mkdir(GamedatPath);
	save.extract(BuildPath(StaticPath, InitGameFileName, 0), GamedatPath);
	LogMemoryUsage("Loading Npcs");
	LoadNpcs(BuildPath(GamedatPath, NPCFileName, 0));
	if (!FindAvatarNpc())
		ReportInvalidSaveGame();
	unlink(BuildPath(GamedatPath, NPCFileName, 0));
	LogMemoryUsage("Loading Regions");
	ForgetSavedRegions();
	LoadGameArgs();
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
unsigned char GameFiles::hasGameDirectory()
{
	unsigned char exists = 0;

	exists = chdir(GamedatPath) == 0;
	if (exists)
		chdir("..");
	return exists;
}

/* The file name of saved game n. */
char *GameFiles::getSaveFileName(int n)
{
	return BuildNumberedPath("", SaveFileNameFormat, n, 0);
}

/* The title of saved game n, or "" when there is none. */
void GameFiles::getSaveTitle(int n, char *title)
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
void GameFiles::restoreGame(int n)
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
void GameFiles::saveGame(int n, char *title, char flag)
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
	unsigned char done;
	struct ffblk ff;

	if (hasGameDirectory()) {
		done = findfirst(BuildPath(GamedatPath, "*.*", 0), &ff, 0);
		while (!done) {
			unlink(BuildPath(GamedatPath, ff.ff_name, 0));
			done = findnext(&ff);
		}
		rmdir(GamedatPath);
	}
}
