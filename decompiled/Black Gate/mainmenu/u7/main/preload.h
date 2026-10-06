#ifndef PRELOAD_H
#define PRELOAD_H

struct AudioOptions;
struct SoundConfig;
struct Speech;

/* Audio settings: each is 0 unset, AUDIO_OFF or AUDIO_ON. */
#define AUDIO_OFF   1
#define AUDIO_ON    2

extern Speech SpeechPlayer;

unsigned char far GetAudioOptions(unsigned char *music, unsigned char *speech, unsigned char *effects);
unsigned char far SetAudioState(unsigned char music, unsigned char speech, unsigned char effects);

extern int CursorX, CursorY;
extern unsigned char CursorDrawn, CursorTracking;
extern char *U7MapFileName;
extern char *U7IregFileFormat;
extern char *U7ChunksFileName;
extern char *ShpDimsFileName;
extern char OptionDelimiters[];
extern char MusicKeyword[];
extern char SpeechKeyword[];
extern char SfxKeyword[];
extern char InterruptKeyword[];
extern char PortKeyword[];
extern char AdlibKeyword[];
extern char RolandKeyword[];
extern char OnKeyword[];
extern char OffKeyword[];
extern char CheatPassword[];
extern unsigned char CheatStart;
extern SoundConfig SoundSetup;
#ifdef __cplusplus
extern "C" {
#endif
void far InitGameSystems();
void far ParseCommandLine(int argc, char **argv);
void far ConfigureSound(char *configuration, char *preferences);
#ifdef __cplusplus
}
#endif
unsigned char far ReadAudioOptions(char *filename, AudioOptions *settings);

#endif
