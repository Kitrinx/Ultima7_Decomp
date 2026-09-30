#ifndef U7SOUND_H
#define U7SOUND_H

#include "adlib.h"

struct SfxNote;
struct Voice;

#define SFX_COUNT           115
#define SFX_NONE            0xff
#define SFX_CHANNELS        4       /* Adlib sound effect buffers */
#define MUSIC_TRACK_COUNT   60
#define MUSIC_NONE          0xff    /* as a track, stops the music */

/* A Roland voice, kept on a ring in order of last use. */
struct VoiceSlot {
	uint8_t number, voice, sound;
	VoiceSlot *next, *previous;
};

/* An Adlib sound effect buffer and the sound playing from it. */
struct SoundChannel {
	uint8_t *buffer;
	uint8_t sound;
	int16_t playback;
	SoundChannel() { buffer = 0; playback = 0; sound = SFX_NONE; }
	uint8_t active() { return playback != 0; }
	uint8_t available() { return sound == SFX_NONE; }
	void release() { StopAdlibVoice(playback); playback = 0; }
	void reset() { playback = 0; sound = SFX_NONE; }
};

extern uint8_t CurrentMusic, DeferredMusic, AdlibSfxActive;
void PlayMusic(uint8_t track);
void StopMusic();

extern uint8_t MusicResume, BackgroundMusicDue, AlternateReverb, SfxEnabled;
extern uint8_t SpecialMusicPlaying;
void PlayPainSfx();
void PlayCombatMusic();
void NoteCombatAlignment(int8_t);
void EnableSfx(int8_t enabled);

extern int16_t AdlibPort;
extern int16_t RolandArgument;
extern VoiceSlot *VoiceSlotHead;
extern SoundChannel SfxChannels[SFX_CHANNELS];
extern char *AdlibMusicFile;
extern char *Mt32MusicFile;
extern char *AdlibSfxFile;
extern char *Mt32SfxFile;
extern uint8_t Mt32MusicVolumes[MUSIC_TRACK_COUNT];
extern uint8_t AdlibMusicVolumes[MUSIC_TRACK_COUNT];
extern uint8_t MusicTrackModes[MUSIC_TRACK_COUNT];
extern int8_t unused_global_5;
extern int16_t RolandVoiceNumbers[32];
extern SfxNote RolandSfxNotes[SFX_COUNT];
extern uint8_t RolandPatchSetup[77];
extern char VoiceFlexName[];
extern char DriverFileName[];
extern uint8_t Mt32AltReverb[3];
extern uint8_t Mt32Reverb[3];
extern uint8_t Mt32PatchEntry[8];
extern uint8_t FirstVoiceChannel;
extern uint8_t *MusicBuffer;
extern uint8_t *SpecialMusicBuffer;
extern char *MusicFileName;
extern int32_t unused_global_6[SFX_COUNT];
extern uint8_t SfxAlternate[SFX_COUNT];
extern Voice *SfxVoices[SFX_COUNT];
extern VoiceSlot VoiceSlots[32];
void SoundTimerHandler();
void TouchVoiceSlot(VoiceSlot *entry);
void ClaimVoiceSlot(VoiceSlot *destination, uint8_t number);
uint8_t ReadMusicRecord(int16_t record, void *destination);
void SetMusicVariant(uint8_t track);
void LoadSfxPatch(uint8_t number);
SoundChannel * FindSfxChannel(uint8_t number);
SoundChannel * AllocateSfxChannel(uint8_t number);
void StopSfx(uint8_t number);
void SetSfxVolume(uint8_t number, int16_t volume);

struct SoundSlotTable;

extern SoundSlotTable SoundSlots;

#endif
