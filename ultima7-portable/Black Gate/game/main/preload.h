#ifndef PRELOAD_H
#define PRELOAD_H

struct AudioOptions;
struct SoundConfig;
struct Speech;

/* Audio settings: each is 0 unset, AUDIO_OFF or AUDIO_ON. */
#define AUDIO_OFF   1
#define AUDIO_ON    2

extern Speech SpeechPlayer;

uint8_t GetAudioOptions(uint8_t *music, uint8_t *speech, uint8_t *effects);
uint8_t SetAudioState(uint8_t music, uint8_t speech, uint8_t effects);

extern int16_t CursorX, CursorY;
extern uint8_t CursorDrawn, CursorTracking;
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
extern "C" uint8_t CheatStart;
extern SoundConfig SoundSetup;
#ifdef __cplusplus
extern "C" {
#endif
void InitGameSystems();
void ParseCommandLine(int16_t argc, char **argv);
void ConfigureSound(char *configuration, char *preferences);
#ifdef __cplusplus
}
#endif
uint8_t ReadAudioOptions(char *filename, AudioOptions *settings);

#endif
