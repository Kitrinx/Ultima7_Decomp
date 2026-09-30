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
	unsigned char number, voice, sound;
	VoiceSlot *next, *previous;
};

/* An Adlib sound effect buffer and the sound playing from it. */
struct SoundChannel {
	int buffer;
	unsigned char sound;
	int playback;
	SoundChannel() { buffer = 0; playback = 0; sound = SFX_NONE; }
	unsigned char active() { return playback != 0; }
	unsigned char available() { return sound == SFX_NONE; }
	void release() { StopAdlibVoice(playback); playback = 0; }
	void reset() { playback = 0; sound = SFX_NONE; }
};

extern unsigned char CurrentMusic, DeferredMusic, AdlibSfxActive;
void far PlayMusic(unsigned char track);
void far StopMusic();

extern unsigned char MusicResume, BackgroundMusicDue, AlternateReverb, SfxEnabled;
extern unsigned char SpecialMusicPlaying;
void far PlayPainSfx();
void far PlayCombatMusic();
void far NoteCombatAlignment(char);
void far EnableSfx(char enabled);

extern int AdlibPort;
extern int RolandArgument;
extern VoiceSlot *VoiceSlotHead;
extern SoundChannel SfxChannels[SFX_CHANNELS];
extern char *AdlibMusicFile;
extern char *Mt32MusicFile;
extern char *AdlibSfxFile;
extern char *Mt32SfxFile;
extern unsigned char Mt32MusicVolumes[MUSIC_TRACK_COUNT];
extern unsigned char AdlibMusicVolumes[MUSIC_TRACK_COUNT];
extern unsigned char MusicTrackModes[MUSIC_TRACK_COUNT];
extern char unused_global_5;
extern int RolandVoiceNumbers[32];
extern SfxNote RolandSfxNotes[SFX_COUNT];
extern unsigned char RolandPatchSetup[77];
extern char VoiceFlexName[];
extern char DriverFileName[];
extern unsigned char Mt32AltReverb[3];
extern unsigned char Mt32Reverb[3];
extern unsigned char Mt32PatchEntry[8];
extern unsigned char FirstVoiceChannel;
extern unsigned char far *MusicBuffer;
extern unsigned char far *SpecialMusicBuffer;
extern char *MusicFileName;
extern long unused_global_6[SFX_COUNT];
extern unsigned char SfxAlternate[SFX_COUNT];
extern Voice far *SfxVoices[SFX_COUNT];
extern VoiceSlot VoiceSlots[32];
void interrupt SoundTimerHandler();
void far TouchVoiceSlot(VoiceSlot *entry);
void far ClaimVoiceSlot(VoiceSlot *destination, unsigned char number);
unsigned char far ReadMusicRecord(int record, void far *destination);
void far SetMusicVariant(unsigned char track);
void far LoadSfxPatch(unsigned char number);
SoundChannel *far FindSfxChannel(unsigned char number);
SoundChannel *far AllocateSfxChannel(unsigned char number);
void far StopSfx(unsigned char number);
void far SetSfxVolume(unsigned char number, int volume);

struct SoundSlotTable;

extern SoundSlotTable SoundSlots;

#endif
