#ifndef MAINMENU_H
#define MAINMENU_H

#include "rgbpal.h"

struct Mouse;
struct MusicSystem;
struct FlexSpeechCache;
class FlexTextPrinter;

/* The folders the game's files are read from, each with its trailing backslash. */
extern char *GamedatPath;
extern char *StaticPath;

/* The music, speech and font every screen shares. */
extern MusicSystem Music;
extern FlexSpeechCache SpeechCache;
extern FlexTextPrinter MenuFont;
extern char *MusicFlex;             /* the song file for the music card */
extern unsigned char SpeechEnabled;

/* Palette fades run this many ticks; a fade sets KeyPressed when a key cuts it short. */
extern int FadeTicks;
extern unsigned char KeyPressed;
extern RgbColor Black;
extern RgbColor Red;

extern Mouse MouseDriver;
extern char AvatarName[];
extern char AvatarSex;              /* 'M' or 'F' */

char *DataPath(char *dir, char *name);
char *OtherDataPath(char *dir, char *name);
unsigned char WaitForKey(unsigned ticks);
void Delay(unsigned ticks);
unsigned char FileExists(char *name);
void CreateFlag(char *name);
void HandleKey(void);
void RestoreSystem(char restorePalette, char clearScreen);
void Quit(void);

#endif
