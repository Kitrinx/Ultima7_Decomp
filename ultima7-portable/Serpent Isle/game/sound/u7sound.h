#ifndef U7SOUND_H
#define U7SOUND_H

#include "ail.h"
#include "systimer.h"

#define SFX_COUNT 136
#define SFX_LAST 135
#define SFX_VOLUME_COUNT 135
#define SFX_NONE 0xff
#define SFX_CHANNELS 6
#define MUSIC_TRACK_COUNT 71
#define MUSIC_NONE 0xff
#define MUSIC_DEVICE_MT32 1
#define MUSIC_DEVICE_ADLIB 2

struct TimbreRecord {
	int8_t patch, bank;
	int32_t offset;
};

extern const uint8_t Mt32MusicVolumes[MUSIC_TRACK_COUNT];
extern const uint8_t AdlibMusicVolumes[MUSIC_TRACK_COUNT];
extern const uint8_t Mt32SfxVolumes[SFX_COUNT];
extern const uint8_t AdlibSfxVolumes[SFX_COUNT];
extern int16_t AdlibPort, RolandArgument;
extern uint8_t MusicResume, BackgroundMusicDue, SfxEnabled, MusicEnabled;
extern uint8_t MusicChangeDue, RequestedMusic, SpecialMusicPlaying;
extern uint16_t SfxAgeCounter;
extern int16_t MusicDevice;
extern char *MusicFileName, *SfxFileName, *TimbreFileName;
extern HSEQUENCE MusicSequence;
extern Timer SfxInterval;
extern uint8_t CurrentMusic, DeferredMusic;
extern uint8_t AdlibSfxActive;
extern const uint8_t MusicTrackModes[MUSIC_TRACK_COUNT];
extern uint16_t TimbreSize;
extern TimbreRecord TimbreEntry;

extern void *MusicBuffer;
extern void *SfxBuffers[SFX_CHANNELS];
extern uint8_t SfxNumbers[SFX_CHANNELS];
extern uint16_t SfxAge[SFX_CHANNELS];
extern HDRIVER MusicDriver;
extern void *MusicStateTable;
extern HSEQUENCE SfxSequences[SFX_CHANNELS];
extern void *SfxStateTables[SFX_CHANNELS];
extern uint8_t *SfxControllerTable;
extern int16_t SfxRelativeVolume[SFX_CHANNELS];

void PlayPainSfx();
void EnableMusic(int16_t enabled);
void * ReadTimbre(int16_t bank, int16_t patch);
void LoadSequenceTimbres(HDRIVER driver, HSEQUENCE sequence, int16_t record);
uint8_t ReadMusicRecord(int16_t record, void *destination);
void PreloadMusicTimbres();
void StopMusicSequence();
void PlayMusic(uint8_t track);
void StopMusic();
void PlayCombatMusic();
void NoteCombatAlignment(int8_t);
uint8_t ReadSfxRecord(int16_t record, void *destination);
void ReleaseSfxChannels(int16_t slot, int8_t percussion);
extern "C" void PlaySfx(uint8_t number, uint16_t volume, int16_t pan);
void StopSfx(uint8_t number, int16_t slot, int8_t percussion);
void SetSfxVolume(uint8_t number, int16_t volume, int16_t previous);
void EnableSfx(int8_t enabled);

#endif
