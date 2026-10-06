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
	char patch, bank;
	long offset;
};

extern unsigned char Mt32MusicVolumes[MUSIC_TRACK_COUNT];
extern unsigned char AdlibMusicVolumes[MUSIC_TRACK_COUNT];
extern unsigned char Mt32SfxVolumes[SFX_VOLUME_COUNT];
extern unsigned char AdlibSfxVolumes[SFX_VOLUME_COUNT];
extern int AdlibPort, RolandArgument;
extern unsigned char MusicResume, BackgroundMusicDue, SfxEnabled, MusicEnabled;
extern unsigned char MusicChangeDue, RequestedMusic, SpecialMusicPlaying;
extern unsigned SfxAgeCounter;
extern int MusicDevice;
extern char *MusicFileName, *SfxFileName, *TimbreFileName;
extern HSEQUENCE MusicSequence;
extern Timer SfxInterval;
extern unsigned char CurrentMusic, DeferredMusic;
extern unsigned char AdlibSfxActive;
extern unsigned char MusicTrackModes[MUSIC_TRACK_COUNT];
extern unsigned TimbreSize;
extern TimbreRecord TimbreEntry;

extern void far *MusicBuffer;
extern void far *SfxBuffers[SFX_CHANNELS];
extern unsigned char SfxNumbers[SFX_CHANNELS];
extern unsigned SfxAge[SFX_CHANNELS];
extern HDRIVER MusicDriver;
extern void far *MusicStateTable;
extern HSEQUENCE SfxSequences[SFX_CHANNELS];
extern void far *SfxStateTables[SFX_CHANNELS];
extern unsigned char far *SfxControllerTable;
extern int SfxRelativeVolume[SFX_CHANNELS];

void far PlayPainSfx();
void far EnableMusic(int enabled);
void far *far ReadTimbre(int bank, int patch);
void far LoadSequenceTimbres(HDRIVER driver, HSEQUENCE sequence, int record);
unsigned char far ReadMusicRecord(int record, void far *destination);
void far PreloadMusicTimbres();
void far StopMusicSequence();
void far PlayMusic(unsigned char track);
void far StopMusic();
void far PlayCombatMusic();
void far NoteCombatAlignment(char);
unsigned char far ReadSfxRecord(int record, void far *destination);
void far ReleaseSfxChannels(int slot, char percussion);
extern "C" void far PlaySfx(unsigned char number, unsigned volume, int pan);
void far StopSfx(unsigned char number, int slot, char percussion);
void far SetSfxVolume(unsigned char number, int volume, int previous);
void far EnableSfx(char enabled);

#endif
