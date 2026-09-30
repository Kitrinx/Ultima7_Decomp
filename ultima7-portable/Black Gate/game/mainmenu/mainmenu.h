#ifndef MAINMENU_MAINMENU_H
#define MAINMENU_MAINMENU_H

#include "../shared/rgbpal.h"

struct Mouse;
struct FlexSpeechCache;

namespace Shared {
struct MusicSystem;
class FlexTextPrinter;
}

namespace MainMenu {

using Shared::RgbColor;
using Shared::MusicSystem;
using Shared::FlexTextPrinter;

/* The folders the game's files are read from, each with its trailing backslash. */
extern char *const GamedatPath;
extern char *const StaticPath;

/* The music, speech and font every screen shares. */
extern MusicSystem Music;
extern FlexSpeechCache MenuSpeech;
extern FlexTextPrinter MenuFont;
extern char *const MusicFlex;       /* the song file for the music card */
extern uint8_t SpeechEnabled;

/* Palette fades run this many ticks; a fade sets KeyPressed when a key cuts it short. */
extern int16_t FadeTicks;
extern uint8_t KeyPressed;
extern RgbColor Black;
extern RgbColor Red;

extern char AvatarName[];
extern char AvatarSex;              /* 'M' or 'F' */

char *DataPath(char *dir, char *name);
char *OtherDataPath(char *dir, char *name);
uint8_t WaitForKey(uint16_t ticks);
void Delay(uint16_t ticks);
uint8_t FileExists(char *name);
void CreateFlag(char *name);
void HandleKey(void);
void RestoreSystem(char restorePalette, char clearScreen);
void Quit(void);

}

#endif
