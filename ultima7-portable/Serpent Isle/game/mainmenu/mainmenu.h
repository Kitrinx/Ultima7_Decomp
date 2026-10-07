#ifndef MAINMENU_H
#define MAINMENU_H

#include "../shared/rgbpal.h"

struct Mouse;

namespace Shared {
struct MusicSystem;
struct FlexSpeechCache;
class FlexTextPrinter;
}

namespace MainMenu {

using Shared::RgbColor;
using Shared::MusicSystem;
using Shared::FlexSpeechCache;
using Shared::FlexTextPrinter;

/* The folders the game's files are read from, each with its trailing backslash. */
extern char *GamedatPath;
extern char *StaticPath;

/* The music, speech and font every screen shares. */
extern MusicSystem Music;
extern FlexSpeechCache SpeechCache;
extern FlexTextPrinter MenuFont;
extern int16_t MusicResourceType;
extern uint8_t SpeechEnabled;

/* Palette fades run this many ticks; a fade sets KeyPressed when a key cuts it short. */
extern int16_t FadeTicks;
extern uint8_t KeyPressed;
extern RgbColor Black;
extern RgbColor Red;

extern Mouse *MouseDriver;
extern char AvatarName[];
extern int8_t AvatarSex;              /* 'M' or 'F' */

char *DataPath(char *dir, char *name);
char *OtherDataPath(char *dir, char *name);
uint8_t WaitForKey(uint16_t ticks);
void Delay(uint16_t ticks);
uint8_t FileExists(char *name);
void CreateFlag(char *name);
void HandleKey(void);
void RestoreSystem(int8_t restorePalette, int8_t clearScreen);
void Quit(void);

}

#endif
