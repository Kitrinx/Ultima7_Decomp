/* Black Gate U7.EXE, resident segment 71 (file offsets 0x0277fd to 0x028454, 3159 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "easyfile.h"
#include "flex.h"
#include "adlib.h"
#include "midiplay.h"
#include "debug.h"
#include "random.h"
#include "attack.h"
#include "crime.h"
#include "u7sound.h"

/* One 8-byte note of a Roland sound effect, as the MIDI player reads it. */
struct SfxNote {
	unsigned char flags;        /* 1 chains to another note, 2 slides, 4 holds, 8 repeats */
	unsigned char patch;        /* program plus one; 0 until its timbre is loaded */
	unsigned char pitch;
	unsigned char velocity;
	int duration;               /* ticks */
	unsigned char slideTo;      /* the pitch a slide ends on */
	unsigned char chain;        /* notes to skip, less one, to reach the chained one */
};

int AdlibPort = 0x388;
int RolandArgument = 2;
unsigned char MusicResume = 0, BackgroundMusicDue = 1, AlternateReverb = 0, SfxEnabled = 1;
VoiceSlot *VoiceSlotHead = 0;
SoundChannel SfxChannels[SFX_CHANNELS];
unsigned char SpecialMusicPlaying = 0;
char *AdlibMusicFile = "adlibmus.dat";
char *Mt32MusicFile = "mt32mus.dat";
char *AdlibSfxFile = "adlibsfx.dat";
char *Mt32SfxFile = "mt32sfx.dat";
unsigned char Mt32MusicVolumes[MUSIC_TRACK_COUNT] = {
	180, 180, 180, 180, 255, 255, 180, 180, 180, 200, 220, 220, 180, 180, 180, 180,
	180, 220, 200, 180, 200, 200, 220, 220, 180, 210, 220, 210, 190, 230, 220, 180,
	220, 250, 220, 200, 220, 220, 220, 200, 180, 240, 220, 240, 220, 220, 220, 220,
	220, 220, 220, 180, 255, 240, 220, 210, 210, 210, 210, 210
};
unsigned char AdlibMusicVolumes[MUSIC_TRACK_COUNT] = {
	180, 180, 180, 180, 255, 255, 180, 180, 180, 180, 180, 180, 180, 180, 180, 180,
	180, 210, 200, 120, 120, 120, 220, 170, 56, 180, 180, 180, 200, 220, 200, 180,
	180, 180, 180, 180, 200, 200, 180, 180, 170, 195, 210, 210, 210, 210, 210, 210,
	210, 210, 210, 180, 255, 180, 180, 190, 190, 190, 190, 210
};
/* 0 background music; 2 a piece the background waits for; 1 one queued to play once after the current song */
unsigned char MusicTrackModes[MUSIC_TRACK_COUNT] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 2,
	2, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
unsigned char CurrentMusic = MUSIC_NONE, DeferredMusic = MUSIC_NONE, AdlibSfxActive = 0;
char unused_global_5 = 0;           /* never referenced */
int RolandVoiceNumbers[32] = {
	2, 3, 4, 8, 9, 10, 11, 20, 21, 22, 29, 36, 44, 45, 46, 48,
	62, 63, 64, 67, 81, 100, 101, 105, 116, 117, 120, 121, 124, 126, 127, 128
};
SfxNote RolandSfxNotes[SFX_COUNT] = {
	{ 0, 0, 60, 127, 45, 0, 0 }, { 0, 0, 60, 127, 5, 0, 0 },
	{ 0, 0, 55, 127, 30, 0, 0 }, { 0, 0, 50, 127, 10, 0, 0 },
	{ 0, 0, 59, 127, 5, 0, 0 }, { 0, 0, 59, 127, 60, 0, 0 },
	{ 0, 0, 64, 127, 30, 0, 0 }, { 0, 0, 60, 127, 50, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 1, 0, 67, 127, 50, 0, 81 }, { 0, 0, 36, 127, 50, 0, 0 },
	{ 0, 0, 60, 96, 45, 0, 0 }, { 0, 0, 60, 127, 600, 0, 0 },
	{ 0, 0, 60, 127, 90, 0, 0 }, { 0, 0, 60, 127, 20, 0, 0 },
	{ 0, 0, 60, 127, 75, 0, 0 }, { 0, 0, 60, 127, 20, 0, 0 },
	{ 0, 0, 60, 127, 20, 0, 0 }, { 0, 0, 60, 127, 70, 0, 0 },
	{ 0, 0, 60, 127, 10, 0, 0 }, { 0, 0, 60, 127, 10, 0, 0 },
	{ 0, 0, 60, 127, 10, 0, 0 }, { 0, 0, 60, 127, 30, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 50, 0, 0 }, { 0, 0, 60, 127, 20, 0, 0 },
	{ 0, 0, 60, 127, 30, 0, 0 }, { 0, 0, 60, 127, 5, 0, 0 },
	{ 0, 0, 60, 64, 20, 0, 0 }, { 1, 0, 60, 64, 15, 0, 61 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 1, 0, 60, 127, 60, 0, 60 },
	{ 0, 0, 53, 127, 60, 0, 0 }, { 1, 0, 60, 127, 60, 0, 59 },
	{ 0, 0, 60, 127, 120, 0, 0 }, { 0, 0, 62, 127, 20, 0, 0 },
	{ 0, 0, 60, 127, 90, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 90, 0, 0 }, { 0, 0, 60, 127, 75, 0, 0 },
	{ 1, 0, 60, 127, 60, 0, 53 }, { 0, 0, 60, 127, 15, 0, 0 },
	{ 0, 0, 60, 127, 15, 0, 0 }, { 0, 0, 60, 127, 70, 0, 0 },
	{ 0, 0, 60, 127, 10, 0, 0 }, { 0, 0, 60, 127, 80, 0, 0 },
	{ 4, 0, 60, 127, 60, 0, 0 }, { 0, 0, 48, 127, 150, 0, 0 },
	{ 4, 0, 60, 127, 60, 0, 0 }, { 4, 0, 60, 64, 60, 0, 0 },
	{ 4, 0, 60, 127, 60, 0, 0 }, { 4, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 50, 0, 0 },
	{ 0, 0, 36, 127, 170, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 36, 127, 50, 0, 0 }, { 0, 0, 60, 127, 100, 0, 0 },
	{ 0, 0, 60, 127, 90, 0, 0 }, { 0, 0, 60, 127, 75, 0, 0 },
	{ 0, 0, 60, 127, 150, 0, 0 }, { 0, 0, 60, 127, 120, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 10, 0, 0 },
	{ 0, 0, 60, 127, 90, 0, 0 }, { 0, 0, 60, 30, 20, 0, 0 },
	{ 0, 0, 62, 30, 20, 0, 0 }, { 0, 0, 60, 24, 30, 0, 0 },
	{ 0, 0, 60, 48, 6, 0, 0 }, { 4, 0, 60, 72, 60, 0, 0 },
	{ 4, 0, 36, 72, 60, 0, 0 }, { 4, 0, 60, 127, 60, 0, 0 },
	{ 4, 0, 72, 127, 60, 0, 0 }, { 4, 0, 60, 127, 60, 0, 0 },
	{ 4, 0, 79, 96, 60, 0, 0 }, { 0, 0, 60, 127, 20, 0, 0 },
	{ 0, 0, 60, 127, 5, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 180, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 70, 0, 0 }, { 0, 0, 60, 127, 75, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 64, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 56, 72, 45, 0, 0 },
	{ 0, 0, 60, 96, 60, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 60, 0, 0 }, { 0, 0, 60, 64, 15, 0, 0 },
	{ 0, 0, 60, 96, 60, 0, 0 }, { 0, 0, 60, 64, 60, 0, 0 },
	{ 0, 0, 60, 64, 600, 0, 0 }, { 0, 0, 72, 96, 180, 0, 0 },
	{ 0, 0, 60, 96, 60, 0, 0 }, { 0, 0, 60, 127, 30, 0, 0 },
	{ 0, 0, 60, 127, 180, 0, 0 }, { 0, 0, 60, 127, 60, 0, 0 },
	{ 0, 0, 60, 127, 120, 0, 0 }, { 0, 0, 60, 127, 75, 0, 0 },
	{ 0, 0, 43, 96, 120, 0, 0 }, { 4, 0, 79, 127, 75, 0, 0 },
	{ 4, 0, 60, 127, 75, 0, 0 }
};
unsigned char RolandPatchSetup[77] = {
	1, 3, 1, 32, 0, 90, 7, 0, 1, 3, 1, 52, 6, 100, 7, 1,
	1, 3, 2, 88, 1, 90, 5, 0, 12, 3, 2, 96, 1, 90, 6, 0,
	1, 90, 7, 0, 2, 100, 7, 1, 1, 90, 8, 0, 5, 90, 7, 1,
	1, 90, 9, 0, 3, 95, 7, 1, 4, 100, 4, 1, 4, 100, 5, 1,
	4, 100, 6, 1, 4, 100, 7, 1, 4, 100, 8, 1, 0
};
char VoiceFlexName[] = "U7VOICE.FLX";
char DriverFileName[] = "U7STRAX.DRV";
unsigned char Mt32AltReverb[3] = { 1, 5, 7 };
unsigned char Mt32Reverb[3] = { 1, 3, 3 };
unsigned char Mt32PatchEntry[8] = { 2, 0, 24, 50, 24, 0, 0, 0 };
VoiceSlot VoiceSlots[32];
unsigned char FirstVoiceChannel;
unsigned char far *MusicBuffer;
unsigned char far *SpecialMusicBuffer;
char *MusicFileName;
void interrupt (*PrevSoundTimer)();
long unused_global_6[SFX_COUNT];            /* never referenced */
unsigned char SfxAlternate[SFX_COUNT];
struct Voice;
Voice far *SfxVoices[SFX_COUNT];

extern "C" void far PlaySfx(int, int, int, int);
extern void (far pascal *DriverSysexEntry)(long, int, void far *);
extern unsigned char SaveLoadActive;

/* The sound slots, one per effect playing. */
struct SoundSlotTable {
	void adjustCount(char id, char up);
};

void far PlayPainSfx()
{
	PlaySfx(GenerateRandomIntegerInRange(5) + 54, 127, 64, 0);
}

void interrupt SoundTimerHandler()
{
	TickAdlibSounds();
	PrevSoundTimer();
}

void far TouchVoiceSlot(VoiceSlot *entry)
{
	if (entry == VoiceSlotHead)
		VoiceSlotHead = entry->next;
	else if (entry->next != VoiceSlotHead) {
		entry->previous->next = entry->next;
		entry->next->previous = entry->previous;
		entry->next = VoiceSlotHead;
		entry->previous = VoiceSlotHead->previous;
		entry->next->previous = entry;
		entry->previous->next = entry;
	}
}

void far ClaimVoiceSlot(VoiceSlot *destination, unsigned char number)
{
	unsigned char old;
	VoiceSlot *current = VoiceSlotHead;

	old = current->sound;
	if (old != SFX_NONE) {
		if (old >= SFX_COUNT)
			CheatPrintfWait("bad sfxnum");
		else {
			if (SfxVoices[old]) {
				StopMidiSfxVoice(SfxVoices[old], old);
				StopMidiSfxVoice(SfxVoices[old], old + 0x8000);
			}
			RolandSfxNotes[old].patch = 0;
		}
	}
	current->sound = number;
	TouchVoiceSlot(current);
	destination->number = current->number;
	destination->voice = current->voice;
}

unsigned char far ReadMusicRecord(int record, void far *destination)
{
	Flex file;

	if (file.open(BuildPath(StaticPath, MusicFileName, 0))) {
		file.readRecord(record, destination, 0);
		file.close();
		return 1;
	} else {
		return 0;
	}
}

void far SetMusicVariant(unsigned char track)
{
	unsigned char alternate = 0;

	if (MusicDevice == MUSIC_DEVICE_MT32) {
		if (track == 24)
			alternate = 1;
		if (alternate != AlternateReverb) {
			AlternateReverb = alternate;
			if (AlternateReverb)
				DriverSysexEntry(0x100001L, 3, Mt32AltReverb);
			else
				DriverSysexEntry(0x100001L, 3, Mt32Reverb);
		}
	}
}

void far PlayMusic(unsigned char track)
{
	int mode;
	int song;

	if (SaveLoadActive != 0)
		return;
	if (MusicDevice == 0)
		return;
	if (IsAvatarDead() && track != 17)
		return;
	song = track;
	MusicResume = 0;
	if (track >= MUSIC_TRACK_COUNT && track != MUSIC_NONE)
		return;
	if (SongStopped) {
		CurrentMusic = DeferredMusic = MUSIC_NONE;
		if (SpecialMusicPlaying)
			SpecialMusicPlaying = 0;
	}
	if (track == MUSIC_NONE)
		mode = 3;
	else
		mode = MusicTrackModes[track];
	if (SpecialMusicPlaying && mode == 0) {
		DeferredMusic = track;
		return;
	}
	if (mode == 1 && ((MusicFlags & MUSIC_RETURN) || CurrentSong == SpecialMusicBuffer)) {
		DeferredMusic = track;
		return;
	}
	if (track == CurrentMusic)
		return;
	if (track == MUSIC_NONE) {
		FadeOutSong(0);
		if (AdlibSfxActive && CurrentMusic != MUSIC_NONE)
			ReclaimAdlibChannels();
	} else {
		if (AdlibSfxActive && CurrentMusic == MUSIC_NONE)
			YieldAdlibChannels();
		if (mode == 1) {
			if (ReadMusicRecord(track, SpecialMusicBuffer))
				QueueSong(SpecialMusicBuffer, MUSIC_RETURN);
			else
				mode = MusicTrackModes[CurrentMusic];
			track = CurrentMusic;
		} else {
			if (CurrentMusic != MUSIC_NONE)
				FadeOutSong(0);
			if (MusicDevice == MUSIC_DEVICE_MT32)
				SetMusicVolume(Mt32MusicVolumes[track]);
			else
				SetMusicVolume(AdlibMusicVolumes[track]);
			if (ReadMusicRecord(track, MusicBuffer)) {
				MusicFlags &= ~MUSIC_RETURN;
				SetMusicVariant(track);
				PlaySong(MusicBuffer);
				BackgroundMusicDue = 0;
			} else {
				track = MUSIC_NONE;
				mode = 0;
				if (AdlibSfxActive)
					ReclaimAdlibChannels();
			}
		}
	}
	CurrentMusic = DeferredMusic = track;
	if (mode == 2 || mode == 1)
		SpecialMusicPlaying = 1;
	else if (mode == 0 || mode == 3)
		SpecialMusicPlaying = 0;
}

void far StopMusic()
{
	MusicResume = 0;
	PlayMusic(MUSIC_NONE);
}

void far PlayCombatMusic()
{
	if (!SpecialMusicPlaying || CurrentMusic == 10) {
		switch (GetDominantHostileSide()) {
		case 1: PlayMusic(12); break;
		case 2: PlayMusic(11); break;
		}
	}
}

void far NoteCombatAlignment(char) {}

void far LoadSfxPatch(unsigned char number)
{
	long address;
	VoiceSlot patch;
	Flex file;
	unsigned char data[246];

	if (RolandSfxNotes[number].patch == 0) {
		if (file.open(BuildPath(StaticPath, Mt32SfxFile, 0))) {
			file.readRecord(number, data, 0);
			file.close();
			ClaimVoiceSlot(&patch, number);
			RolandSfxNotes[number].patch = patch.voice + 1;
			/* keep the music timer off the driver while the timbre is sent */
			MusicDevice = 0;
			Mt32PatchEntry[1] = patch.number;
			address = (patch.number << 9) + 0x80000L;
			DriverSysexEntry(address, 246, data);
			address = ((patch.voice << 3) & 0x7f) + (long)((patch.voice << 4) & 0x7f00) + 0x50000L;
			DriverSysexEntry(address, 8, Mt32PatchEntry);
			MusicDevice = MUSIC_DEVICE_MT32;
		}
	}
}

SoundChannel *far FindSfxChannel(unsigned char number)
{
	int i;

	for (i = 0; i < SFX_CHANNELS; i++)
		if (SfxChannels[i].sound == number)
			return &SfxChannels[i];
	return 0;
}

SoundChannel *far AllocateSfxChannel(unsigned char number)
{
	int i, score;
	SoundChannel *selected;
	int best, highest;
	Flex file;

	selected = 0;
	best = SFX_CHANNELS;
	selected = FindSfxChannel(number);
	if (selected) {
		if (selected->active())
			selected->release();
	} else {
		for (i = 0; i < SFX_CHANNELS; i++) {
			if (SfxChannels[i].available()) {
				best = i;
				break;
			}
		}
		if (best == SFX_CHANNELS) {
			highest = 0;
			for (i = 0; i < SFX_CHANNELS; i++) {
				score = 0;
				if (SfxChannels[i].playback) {
					switch (GetAdlibVoiceState(SfxChannels[i].playback)) {
					case 0: score = 6; break;
					case 1: score = 4; break;
					case 2: score = 2; break;
					}
					if (RolandSfxNotes[SfxChannels[i].sound].flags == 4)
						score--;
				} else
					score = 7;
				if (score > highest) {
					highest = score;
					best = i;
				}
			}
			if (best < SFX_CHANNELS) {
				if (SfxChannels[best].active())
					SfxChannels[best].release();
				SfxChannels[best].reset();
			}
		}
		if (best < SFX_CHANNELS) {
			if (file.open(BuildPath(StaticPath, AdlibSfxFile, 0))) {
				if (file.readRecord(number, MK_FP(SfxChannels[best].buffer, 0), 1)) {
					SfxChannels[best].sound = number;
					selected = &SfxChannels[best];
				}
				file.close();
			}
		}
	}
	return selected;
}

/* Callers declare number as unsigned char, so its high byte is whatever AH held. */
extern "C" void far PlaySfx(int number, int volume, int pan, int flags)
{
	unsigned char other;
	int v;
	SoundChannel *c;

	if (!SfxEnabled)
		return;
	if ((unsigned char) number >= SFX_COUNT)
		return;
	if (MusicDevice == MUSIC_DEVICE_MT32) {
		LoadSfxPatch(number);
		other = SFX_NONE;
		switch ((unsigned char) number) {
		case 10: other = 92; break;
		case 31: other = 93; break;
		case 33: other = 94; break;
		case 35: other = 95; break;
		case 42: other = 96; break;
		}
		if (other != SFX_NONE) {
			LoadSfxPatch(other);
			SfxVoices[other] = 0;
		}
		v = number;
		if (SfxAlternate[(unsigned char) number] && (flags & 1))
			v += 0x8000;
		if (flags & 2)
			RolandSfxNotes[(unsigned char) number].pitch =
				GenerateRandomIntegerInRange(5) - GenerateRandomIntegerInRange(5) + 60;
		SfxVoices[(unsigned char) number] =
			StartMidiSfx((unsigned char far *)&RolandSfxNotes[(unsigned char) number], 0, volume, pan, v, 0);
		SfxAlternate[(unsigned char) number] = !SfxAlternate[(unsigned char) number];
	}
	if (AdlibSfxActive) {
		c = AllocateSfxChannel(number);
		if (c)
			c->playback = StartAdlibSound(c->buffer, volume);
		SoundSlots.adjustCount(number, 1);
	}
}

void far StopSfx(unsigned char number)
{
	SoundChannel *current;

	if (!SfxEnabled)
		return;
	if (number >= SFX_COUNT)
		return;
	if (MusicDevice == MUSIC_DEVICE_MT32 && SfxVoices[number] != 0) {
		StopMidiSfxVoice(SfxVoices[number], number);
		StopMidiSfxVoice(SfxVoices[number], number + 0x8000);
		SfxVoices[number] = 0;
	}
	if (AdlibSfxActive != 0) {
		current = FindSfxChannel(number);
		if (current != 0) {
			if (current->active())
				current->release();
			current->reset();
		}
		SoundSlots.adjustCount(number, 0);
	}
}

void far SetSfxVolume(unsigned char number, int volume)
{
	SoundChannel *current;

	if (!SfxEnabled)
		return;
	if (number >= SFX_COUNT)
		return;
	if (MusicDevice == MUSIC_DEVICE_MT32 && SfxVoices[number] != 0) {
		SetMidiSfxControl(SfxVoices[number], 7, volume, number);
		SetMidiSfxControl(SfxVoices[number], 7, volume, number + 0x8000);
	}
	if (AdlibSfxActive) {
		current = FindSfxChannel(number);
		if (current != 0 && current->active())
			SetAdlibVolume(current->playback, volume);
	}
}

void far EnableSfx(char enabled)
{
	int i;

	if (enabled)
		SfxEnabled = 1;
	else if (SfxEnabled) {
		if (MusicDevice == MUSIC_DEVICE_MT32)
			DropAllMidiSfx();
		if (AdlibSfxActive) {
			for (i = 0; i < SFX_CHANNELS; i++) {
				if (SfxChannels[i].active())
					SfxChannels[i].release();
				SfxChannels[i].reset();
			}
		}
		SfxEnabled = 0;
	}
}
