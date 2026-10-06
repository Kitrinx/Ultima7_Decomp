#ifndef DEBUG_H
#define DEBUG_H

extern char *GameTitle;
extern char *GameVersion;
extern char *GameCopyright;
extern char *StaticPath;
extern char *GamedatPath;
extern char *BuildStamp;
extern unsigned char ShouldExitMainGameLoop;
extern unsigned char DebugOutputEnabled;
extern char TitleString[];
extern char VersionString[];
extern char CopyrightString[];
extern char BuildStampString[];

/* Debug and status text output. */
void far DebugPrintf(char *fmt, ...);
void far DebugPrintfAtCoords(int x, int y, char *fmt, ...);
void DebugPrintTgCode(int code);
void far DebugPrintfWait(char *fmt, ...);
void far DebugPrintfAtCoordsWait(int x, int y, char *fmt, ...);
void DebugPrintTgCodeWait(int code);
void far CheatPrintf(char *fmt, ...);
void far CheatPrintfAtCoords(int x, int y, char *fmt, ...);
void far CheatPrintfWait(char *fmt, ...);
void CheatPrintfAtCoordsWait(int x, int y, char *fmt, ...);

/* Defined in the overlay profiler and the coordinate display. */
#ifdef __cplusplus
extern "C" {
#endif
unsigned long GetOverlayLogCount(void);
unsigned long GetOverlayLoadCount(void);
#ifdef __cplusplus
}
#endif

extern char CoordFormat[];
void ShowScreenCoords(void);

#endif
