#ifndef DEBUG_H
#define DEBUG_H

extern char *const GameTitle;
extern char *const GameVersion;
extern char *const GameCopyright;
extern char *StaticPath;
extern char *GamedatPath;
extern char *const BuildStamp;
extern uint8_t ShouldExitMainGameLoop;
extern uint8_t DebugOutputEnabled;
extern const char TitleString[];
extern const char VersionString[];
extern const char CopyrightString[];
extern const char BuildStampString[];

/* Debug and status text output. */
void DebugPrintf(char *fmt, ...);
void DebugPrintfAtCoords(int16_t x, int16_t y, char *fmt, ...);
void DebugPrintTgCode(int16_t code);
void DebugPrintfWait(char *fmt, ...);
void DebugPrintfAtCoordsWait(int16_t x, int16_t y, char *fmt, ...);
void DebugPrintTgCodeWait(int16_t code);
void CheatPrintf(char *fmt, ...);
void CheatPrintfAtCoords(int16_t x, int16_t y, char *fmt, ...);
void CheatPrintfWait(char *fmt, ...);
void CheatPrintfAtCoordsWait(int16_t x, int16_t y, char *fmt, ...);
/* printf and gotoxy with cprintf, as the DOS game used them: text on the game screen. */
void ConsoleWrite(const char *text);
void ConsoleWriteAt(int16_t x, int16_t y, const char *text);
/* The next word or number typed on the game screen; blank lines are skipped. */
void ConsoleReadWord(char *word, int16_t size);
int32_t ConsoleReadNumber(int8_t hex);

/* Defined in the coordinate display. */
extern const char CoordFormat[];
void ShowScreenCoords(void);

#endif
