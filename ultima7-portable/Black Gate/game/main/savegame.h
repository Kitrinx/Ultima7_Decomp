#ifndef SAVEGAME_H
#define SAVEGAME_H

/* The saved-game files and the game directory. */
struct GameFiles {
	int16_t getFreeSaveSpace();
	void saveAll(char *dir, int8_t flag);
	void loadAll(char *dir);
	void loadGame();
	uint8_t hasGameDirectory();
	char *getSaveFileName(int16_t n);
	void getSaveTitle(int16_t n, char *title);
	void restoreGame(int16_t n);
	void saveGame(int16_t n, char *title, int8_t flag);
	void deleteGameDirectory();
};

extern char *const InitGameFileName;
extern char *const SaveGameFileName;
extern char *const SaveFileNameFormat;
extern char *const NPCFileName;
int32_t GetDiskFreeBytes(void);
int8_t MakeNewGame(void);

extern GameFiles SaveGameFiles;
extern uint8_t SaveLoadActive;

#endif
