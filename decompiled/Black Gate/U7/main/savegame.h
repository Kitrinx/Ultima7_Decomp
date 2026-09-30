#ifndef SAVEGAME_H
#define SAVEGAME_H

/* The saved-game files and the game directory. */
struct GameFiles {
	int getFreeSaveSpace();
	void saveAll(char *dir, char flag);
	void loadAll(char *dir);
	void loadGame();
	unsigned char hasGameDirectory();
	char *getSaveFileName(int n);
	void getSaveTitle(int n, char *title);
	void restoreGame(int n);
	void saveGame(int n, char *title, char flag);
	void deleteGameDirectory();
};

extern char *InitGameFileName;
extern char *SaveGameFileName;
extern char *SaveFileNameFormat;
extern char *NPCFileName;
long GetDiskFreeBytes(void);
char MakeNewGame(void);

extern GameFiles SaveGameFiles;
extern unsigned char SaveLoadActive;

#endif
