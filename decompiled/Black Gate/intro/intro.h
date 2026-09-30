#ifndef INTRO_H
#define INTRO_H

#include "rgbpal.h"

struct MusicSystem;
struct FileSpeechCache;
struct AudioOptions;

/* The folders the game's files are read from, each with its trailing backslash. */
extern char *StaticPath;
extern char *GamedatPath;

/* How many screen paints the machine manages in a measured time; motion is scaled by it. */
extern int SpeedDivisor;

/* The speech, the music and the song file for the music card. */
extern FileSpeechCache SpeechCache;
extern MusicSystem Music;
extern RgbColor Black;
extern RgbColor Red;
extern char *MusicFlex;
extern unsigned char SpeechEnabled;

char *DataPath(char *dir, char *name);
unsigned char WaitForKey(unsigned ticks);
void Delay(unsigned ticks);
char *OtherDataPath(char *dir, char *name);

/* The six scenes of the introduction, in the order they play. */
void ShowPresents(int ticks, int fadeInDelay, int fadeOutDelay);
void ShowTitle(int ticks, int unusedTicks, int fadeInDelay, int speed, int unusedFlag);
void ShowStatic(int firstFrames, int secondFrames, int thirdFrames, int firstPause, int secondPause);
void ShowGuardian(int cycles, int delay, int firstInRate, int firstOutRate, int secondInRate,
	int secondOutRate, int unusedRate, char *trackFile, int trackEntry, int staticFrames, int pause,
	int endPause, int lineTicks);
void ShowDesk(int ticks, int captionTicks, int panTicks, int captionDelay, int delay);
void ShowMoongate(int delay, int glowCycles, int unusedCycles, int captionDelay, int endCycles);

void InstallFatalHook(void);
void RestoreSystem(char restorePalette);
void HandleKey(void);
void CloseDevices(void);
void QuitToDos(void);
void ReturnToMenu(void);
void Quit(void);
void OnFatalError(void);
void InitEnvironment(void);
unsigned char ReadAudioOptions(char *filename, AudioOptions *settings);
void ConfigureSound(char *configuration, char *preferences, int *irq, int *port, unsigned char *device, int *dma);
int CheckInstallation(void);
void MeasureSpeed(void);

#endif
