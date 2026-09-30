#ifndef DEBUG_H
#define DEBUG_H

extern char *GameTitle;
extern char *GameVersion;
extern char *GameCopyright;
extern char *StaticPath;
extern char *GamedatPath;
extern char *BuildStamp;
extern uint8_t ShouldExitMainGameLoop;
extern uint8_t DebugOutputEnabled;
extern char TitleString[];
extern char VersionString[];
extern char CopyrightString[];
extern char BuildStampString[];

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

/* Defined in the coordinate display. */
extern char CoordFormat[];
void ShowScreenCoords(void);

#endif
