/* Serpent Isle MAINMENU.EXE, resident segment 44 (file offsets 0x012d9b to 0x014aa3, 7432 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d -vi- -y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "flex.h"
#include "systimer.h"
#include "memapi.h"
#include "midiplay.h"
#include "init.h"
#include "plat.h"
#include "mt32drv.h"

namespace Shared {

/*
 * The MIDI player: sound effects on borrowed channels, and a Standard MIDI File
 * song played from the 60 Hz sound tick through the sound driver.
 *
 *   game --> start/stop/control effect --> voices[32] --\
 *   game --> play/queue/fade song -------> tracks[32] ---+--> driver entry points
 *   sound tick -------> events due --> tracks ----------/
 */

#define SFX_VOICES          32
#define MAX_TRACKS          32
#define FREE_CHANNELS       11      /* the heads of the free and busy channel lists */
#define BUSY_CHANNELS       12
#define CHANNEL_LINKS       13
#define PERCUSSION_CHANNEL  10      /* MIDI channel 10, counted from 1 */

/* MIDI events, and the controllers the player uses */
#define MIDI_NOTE_OFF       0x80
#define MIDI_NOTE_ON        0x90
#define MIDI_AFTERTOUCH     0xa0
#define MIDI_CONTROL        0xb0
#define MIDI_PROGRAM        0xc0
#define MIDI_PITCH_BEND     0xe0
#define MIDI_SYSEX          0xf0
#define MIDI_SYSEX_END      0xf7
#define MIDI_SONG_EVENT     0xfe    /* the game's own loops and branches */
#define MIDI_META           0xff
#define META_END_OF_TRACK   0x2f
#define META_TEMPO          0x51
#define CONTROL_VOLUME      7
#define CONTROL_PAN         10
#define CONTROL_EXIT        0x50    /* where a song may stop once another is queued */
#define CONTROL_LOOP_START  0x51
#define CONTROL_LOOP_END    0x52
#define CONTROL_RESET       0x79
#define CONTROL_NOTES_OFF   0x7b

/* The game's own song events: loops, exits, markers and branches. */
#define SONG_LOOP           0       /* jump back, unless an exit is pending */
#define SONG_EXIT           1       /* stop here once another song is queued */
#define SONG_EXIT_LOOP      2       /* jump back only once an exit is pending */
#define SONG_MARKER         3
#define SONG_BRANCH_UNLESS  4       /* jump back unless the branch value matches */
#define SONG_BRANCH_IF      5       /* jump back when it matches */

/* Who holds a channel. */
#define OWNER_NONE          0
#define OWNER_SONG          1
#define OWNER_SFX           2

/* Voice flags */
#define VOICE_STEREO        1       /* plays on two channels, panned apart */
#define VOICE_KEEP_CHANNEL  2

/* Flags in the first byte of each 8-byte effect note. */
#define NOTE_CHAIN          1       /* go on to the note byte 7, plus one, notes ahead */
#define NOTE_SLIDE          2       /* the pitch steps toward byte 6 */
#define NOTE_HOLD           4
#define NOTE_REPEAT         8

typedef void ( *Routine)();

/* One sound effect: a list of 8-byte notes played on one channel, or two for stereo. */
struct Voice {
	uint8_t flags;
	uint8_t channel;
	uint8_t channel2;
	uint8_t note;
	uint8_t volume;
	uint8_t pan;
	int16_t id;
	int16_t duration;
	uint8_t *data;
};

/* Holds the sound lock for a scope: game calls that share state with the sound tick. */
struct SoundLock {
	SoundLock() { plat_sound_lock(); }
	~SoundLock() { plat_sound_unlock(); }
};

SoundDriver *SoundDriverInfo;
SoundDriver *( *DriverDescribeEntry)(void);
int16_t ( *DriverInitEntry)(void *);
void ( *DriverShutdownEntry)(void);
Routine DriverTimerEntry;
void ( *DriverNoteEntry)(int16_t, int16_t, int16_t);
void ( *DriverControlEntry)(int16_t, int16_t, int16_t);
void ( *DriverPitchEntry)(int16_t, int16_t, int16_t);
void ( *DriverProgramEntry)(int16_t, int16_t);
void ( *DriverPortEntry)(int16_t);
void ( *DriverSysexEntry)(int32_t, int16_t, void *);
void ( *DriverReleaseChannelEntry)(void);
void ( *DriverClaimChannelEntry)(void);
void (*SoundTickFirst)(void) = 0;
void *TimbreBank;
char unused_global_7[20];       /* never referenced */
uint8_t *LoopTrackPos[MAX_TRACKS];
uint8_t LoopTrackStatus[MAX_TRACKS];
int32_t LoopTrackClock[MAX_TRACKS];
int16_t LoopTrackDone[MAX_TRACKS];
uint8_t *TrackPos[MAX_TRACKS];
uint8_t TrackStatus[MAX_TRACKS];
int32_t TrackClock[MAX_TRACKS];
int16_t TrackDone[MAX_TRACKS];
int16_t SongFormat;
int16_t TrackCount;
int16_t TicksPerBeat;

int16_t MusicDevice = 0;
int16_t MidiSfxReady = 0;
int16_t SongMarker = 0;
int16_t SongBranchValue = 0;
int16_t SongFromStart = 1;
int16_t MusicOn = 1;
int16_t SongVolume = 256;
int16_t MusicVolume = 256;
int16_t FadeStep = 0;
int16_t FadeLevel = 0;
int16_t FadeKeepsSong = 0;
int16_t SongExitPending = 0;
uint8_t *QueuedSong = 0;
uint8_t *CurrentSong = 0;
Voice MidiSfxVoices[SFX_VOICES] = { 0 };
uint8_t ChannelProgram[11] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
uint8_t ChannelVolume[11] = { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
uint8_t ChannelPan[11] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };
uint8_t SongProgram[11] = { 0 };

/* Channels 2-9 for effects, each linked to the next in its list; the two list heads follow. */
static const uint8_t ChannelLinksStart[CHANNEL_LINKS] = { 0, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10, 2, 12 };
uint8_t ChannelLinks[CHANNEL_LINKS];
char ChannelPriority[CHANNEL_LINKS] = { 0 };
uint8_t ChannelOwner[CHANNEL_LINKS] = { 0 };
uint32_t TickLength = 10000;
int32_t SongClock = 0;

/* On Adlib, percussion notes are played as this channel and note. */
const uint8_t PercussionChannel[88] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0,
	0, 13, 0, 10, 10, 29, 11, 0, 0, 0, 27, 0, 16, 0, 14, 0,
	0, 15, 0, 30, 0, 0, 28, 0, 21, 0, 0, 0, 26, 26, 25, 20,
	20, 29, 29, 21, 21, 22, 0, 30, 30, 24, 17, 20, 17, 17, 18, 17,
	19, 17, 23, 0, 0, 0, 0, 0
};
const uint8_t PercussionNote[88] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 0, 0, 0,
	0, 33, 0, 48, 48, 48, 48, 0, 0, 0, 71, 0, 71, 0, 71, 0,
	0, 79, 0, 77, 0, 0, 54, 0, 71, 0, 0, 0, 72, 79, 79, 64,
	58, 65, 66, 89, 84, 48, 0, 71, 72, 36, 86, 96, 90, 92, 66, 96,
	80, 100, 82, 0, 0, 0, 0, 0
};
int16_t SongStopped = 1;
int16_t MusicFlags = 0;

/* Relink channel n in front of whichever entry links to next. */
static void LinkMidiChannel(int16_t n, int16_t next)
{
	int16_t i;

	for (i = 0; i < CHANNEL_LINKS; i++) {
		if (ChannelLinks[i] == next) {
			ChannelLinks[i] = n;
			ChannelLinks[n] = next;
			break;
		}
	}
}

/* Move channel n to the end of the busy list at the given priority. */
static void ClaimMidiChannel(int16_t n, int16_t priority)
{
	int16_t i;

	if (n >= 2 && n <= 9) {
		for (i = 0; i < CHANNEL_LINKS; i++) {
			if (ChannelLinks[i] == n) {
				ChannelLinks[i] = ChannelLinks[n];
				ChannelLinks[n] = n;
				LinkMidiChannel(n, BUSY_CHANNELS);
				ChannelPriority[n] = priority;
				break;
			}
		}
		if (MidiSfxReady != 0) {
			MidiSfxReady = 0;
			for (i = 0; i < SFX_VOICES; i++) {
				if (MidiSfxVoices[i].channel == n)
					MidiSfxVoices[i].channel = 0;
			}
			MidiSfxReady = 1;
		}
	}
}

/* Silence channel n and return it to the free list. */
void FreeMidiChannel(int16_t n)
{
	int16_t i;

	if (n >= 2 && n <= 9) {
		plat_sound_lock();
		DriverControlEntry(n, CONTROL_NOTES_OFF, 0);
		DriverControlEntry(n, CONTROL_RESET, 0);
		plat_sound_unlock();
		ChannelOwner[n] = OWNER_NONE;
		for (i = 0; i < CHANNEL_LINKS; i++) {
			if (ChannelLinks[i] == n) {
				ChannelLinks[i] = ChannelLinks[n];
				ChannelLinks[n] = n;
				LinkMidiChannel(n, FREE_CHANNELS);
				break;
			}
		}
	}
}

/* Take a free channel, or the busy one of lowest priority not above priority. */
int16_t AllocateMidiChannel(int16_t priority)
{
	int16_t n, result;

	result = 0;
	if (ChannelLinks[FREE_CHANNELS] != FREE_CHANNELS)
		result = ChannelLinks[FREE_CHANNELS];
	else {
		n = ChannelLinks[BUSY_CHANNELS];
		while (n != BUSY_CHANNELS) {
			if (ChannelPriority[n] <= priority) {
				result = n;
				break;
			}
			n = ChannelLinks[n];
		}
	}
	ClaimMidiChannel(result, priority);
	return result;
}

/* Start the next note of a sound effect on its channels. */
void StartSfxNote(int16_t n)
{
	uint8_t *p;
	int16_t note, velocity, duration;
	int16_t volume, volume2, pan, pan2;
	Voice *v;
	int16_t program;

	if (MusicDevice != 0) {
		v = &MidiSfxVoices[n];
		p = v->data;
		program = p[1];
		if (program != 0 && v->channel != 0) {
			note = p[2];
			velocity = p[3];
			duration = *(int16_t *)(p + 4);
			if (v->flags & VOICE_STEREO) {
				pan = 0;
				pan2 = 127;
				volume = v->volume * (v->pan < 32 ? 127 : 159 - v->pan) >> 7;
				volume2 = v->volume * (v->pan > 95 ? 127 : v->pan + 32) >> 7;
			} else {
				volume = volume2 = v->volume;
				pan = pan2 = v->pan;
			}
			plat_sound_lock();
			if (ChannelOwner[v->channel] != OWNER_SFX) {
				DriverControlEntry(v->channel, CONTROL_NOTES_OFF, 0);
				DriverControlEntry(v->channel, CONTROL_RESET, 0);
				ChannelOwner[v->channel] = OWNER_SFX;
			}
			DriverProgramEntry(v->channel, program - 1);
			ChannelProgram[v->channel] = program - 1;
			DriverControlEntry(v->channel, CONTROL_VOLUME, volume);
			DriverControlEntry(v->channel, CONTROL_PAN, pan);
			DriverNoteEntry(v->channel, note, velocity);
			if ((v->flags & VOICE_STEREO) && v->channel2 != 0) {
				if (ChannelOwner[v->channel2] != OWNER_SFX) {
					DriverControlEntry(v->channel2, CONTROL_NOTES_OFF, 0);
					DriverControlEntry(v->channel2, CONTROL_RESET, 0);
					ChannelOwner[v->channel2] = OWNER_SFX;
				}
				DriverProgramEntry(v->channel2, program - 1);
				ChannelProgram[v->channel2] = program - 1;
				DriverControlEntry(v->channel2, CONTROL_VOLUME, volume2);
				DriverControlEntry(v->channel2, CONTROL_PAN, pan2);
				DriverNoteEntry(v->channel2, note, velocity);
			}
			plat_sound_unlock();
			v->note = note;
			v->duration = duration;
		} else {
			FreeMidiChannel(v->channel);
			if (v->flags & VOICE_STEREO)
				FreeMidiChannel(v->channel2);
			if (!(v->flags & VOICE_KEEP_CHANNEL))
				v->channel = 0;
		}
	}
}

/* Advance every sounding effect by one tick. */
void TickMidiSfx(void)
{
	int16_t flags, changed;
	Voice *v;
	int16_t i;

	if (MusicDevice != 0 && MidiSfxReady != 0) {
		for (i = 0; i < SFX_VOICES; i++) {
			changed = 0;
			if ((v = &MidiSfxVoices[i])->data != 0 && v->channel != 0) {
				flags = *v->data;
				if (!(flags & NOTE_HOLD) && --v->duration == 0) {
					DriverNoteEntry(v->channel, v->note, 0);
					if (v->flags & VOICE_STEREO)
						DriverNoteEntry(v->channel2, v->note, 0);
					if (flags & NOTE_SLIDE) {
						if (v->data[6] > v->note) {
							v->note++;
							changed = 1;
						} else if (v->data[6] < v->note) {
							v->note--;
							changed = 1;
						} else
							v->note = v->data[2];
					}
					if (flags & NOTE_REPEAT)
						changed = 1;
					if (changed) {
						DriverNoteEntry(v->channel, v->note, v->data[3]);
						v->duration = *(int16_t *)(v->data + 4);
					} else if (flags & NOTE_CHAIN) {
						v->data += (v->data[7] + 1) * 8;
						StartSfxNote(i);
					} else {
						FreeMidiChannel(v->channel);
						if (v->flags & VOICE_STEREO)
							FreeMidiChannel(v->channel2);
						if (!(v->flags & VOICE_KEEP_CHANNEL))
							v->channel = 0;
					}
				}
			}
		}
	}
}

/* Start a sound effect, replacing any playing under the same id. */
Voice *StartMidiSfx(uint8_t *data, int16_t flags, int16_t volume, int16_t pan, int16_t id, int16_t priority)
{
	Voice *v;
	int16_t channel = 0, channel2 = 0;
	int16_t free = -1;
	int16_t i;
	SoundLock hold;

	if (MusicDevice != 0) {
		for (i = 0; i < SFX_VOICES; i++) {
			if (MidiSfxVoices[i].channel != 0 && MidiSfxVoices[i].id == id) {
				channel = MidiSfxVoices[i].channel;
				channel2 = MidiSfxVoices[i].channel2;
				plat_sound_lock();
				DriverNoteEntry(channel, MidiSfxVoices[i].note, 0);
				if (MidiSfxVoices[i].flags & VOICE_STEREO)
					DriverNoteEntry(channel2, MidiSfxVoices[i].note, 0);
				plat_sound_unlock();
				MidiSfxVoices[i].channel = 0;
				break;
			}
			if (MidiSfxVoices[i].channel == 0)
				free = i;
		}
		if (channel == 0 && free >= 0) {
			i = free;
			channel = AllocateMidiChannel(priority);
			if (channel != 0 && (flags & VOICE_STEREO))
				channel2 = AllocateMidiChannel(priority);
		}
		if (channel != 0 && (channel2 != 0 || !(flags & VOICE_STEREO))) {
			v = &MidiSfxVoices[i];
			v->flags = flags;
			v->channel = channel;
			v->channel2 = channel2;
			v->volume = volume;
			v->pan = pan;
			v->id = id;
			v->data = data;
			StartSfxNote(i);
			return v;
		}
	}
	return 0;
}

/* Change a controller of a sounding effect. */
void SetMidiSfxControl(Voice *v, int16_t control, int16_t value, int16_t id)
{
	int16_t volume, volume2;
	SoundLock hold;

	if (MusicDevice == 0 || v == 0 || (id != 0 && v->id != id) || v->channel == 0)
		return;
	if (control == CONTROL_VOLUME)
		v->volume = value;
	if (control == CONTROL_PAN)
		v->pan = value;
	if ((v->flags & VOICE_STEREO) && control == CONTROL_PAN) {
		volume = v->volume * (v->pan < 32 ? 127 : 159 - v->pan) >> 7;
		volume2 = v->volume * (v->pan > 95 ? 127 : v->pan + 32) >> 7;
		plat_sound_lock();
		if (ChannelOwner[v->channel] == OWNER_SFX)
			DriverControlEntry(v->channel, CONTROL_VOLUME, volume);
		if (ChannelOwner[v->channel2] == OWNER_SFX)
			DriverControlEntry(v->channel2, CONTROL_VOLUME, volume2);
		plat_sound_unlock();
		return;
	}
	plat_sound_lock();
	if (ChannelOwner[v->channel] == OWNER_SFX)
		DriverControlEntry(v->channel, control, value);
	if ((v->flags & VOICE_STEREO) && ChannelOwner[v->channel2] == OWNER_SFX)
		DriverControlEntry(v->channel2, control, value);
	plat_sound_unlock();
}

/* Give a channel back to the music, restoring its program, volume and pan. */
void ReturnChannelToSong(int16_t channel)
{
	if (MusicDevice != 0 && ChannelOwner[channel] == OWNER_SFX) {
		FreeMidiChannel(channel);
		plat_sound_lock();
		DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
		DriverControlEntry(channel, CONTROL_RESET, 0);
		DriverProgramEntry(channel, SongProgram[channel]);
		ChannelProgram[channel] = SongProgram[channel];
		DriverControlEntry(channel, CONTROL_VOLUME, ChannelVolume[channel] * SongVolume >> 8);
		DriverControlEntry(channel, CONTROL_PAN, ChannelPan[channel]);
		ChannelOwner[channel] = OWNER_SONG;
		plat_sound_unlock();
	}
}

/* Stop a sound effect and return its channels to the music. */
void StopMidiSfxVoice(Voice *v, int16_t id)
{
	SoundLock hold;

	if (MusicDevice != 0 && MidiSfxReady != 0) {
		MidiSfxReady = 0;
		if (v != 0 && (id == 0 || v->id == id) && v->channel != 0) {
			ReturnChannelToSong(v->channel);
			if (v->flags & VOICE_STEREO)
				ReturnChannelToSong(v->channel2);
			v->channel = 0;
		}
		MidiSfxReady = 1;
	}
}

/* Stop one sound effect, or all of them when v is null. */
void StopMidiSfx(Voice *v, int16_t id)
{
	int16_t i;

	if (MusicDevice != 0) {
		if (v == 0) {
			for (i = 0; i < SFX_VOICES; i++)
				StopMidiSfxVoice(&MidiSfxVoices[i], id);
		} else
			StopMidiSfxVoice(v, id);
	}
}

/* Drop every sound effect. */
void DropAllMidiSfx(void)
{
	int16_t i;
	SoundLock hold;

	if (MusicDevice != 0 && MidiSfxReady != 0) {
		MidiSfxReady = 0;
		for (i = 0; i < SFX_VOICES; i++)
			MidiSfxVoices[i].channel = 0;
		for (i = 2; i < 10; i++)
			ReturnChannelToSong(i);
		MidiSfxReady = 1;
	}
}

/* Read a variable-length number from a track. */
int32_t ReadVarLength(int16_t track)
{
	int32_t v = 0;

	while (*TrackPos[track] & 0x80)
		v = (v << 7) + (*TrackPos[track]++ & 0x7f);
	v = (v << 7) + *TrackPos[track]++;
	return v;
}

/* Whether the chunk at p starts with the four-character id. */
int16_t MatchChunkId(uint8_t *p, char *id)
{
	int16_t i;

	for (i = 0; i < 4; i++)
		if (*p++ != *id++)
			break;
	return i == 4;
}

/* The chunk's length, stored big-endian after its id. */
int16_t GetChunkLength(uint8_t *p)
{
	int32_t len;

	len = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	return len;
}

/* A meta event: take up a tempo change, skip the rest. */
void HandleMetaEvent(int16_t track, int16_t type, int16_t len)
{
	int32_t tempo;
	int16_t i;

	switch (type) {
	case META_TEMPO:
		tempo = 0;
		for (i = 0; i < len; i++) {
			tempo <<= 8;
			tempo += TrackPos[track][i];
		}
		TickLength = tempo / TicksPerBeat;
		TickLength *= 4887;
		TickLength /= 1243;
		break;
	}
	TrackPos[track] += len;
}

/* Add a delta time to a track's clock. */
void AddTrackDelta(int16_t track, int32_t delta)
{
	TrackClock[track] += delta;
}

/* The unfinished track whose next event comes soonest; the time until then passes on every track. */
int16_t NextDueTrack(void)
{
	int16_t best;
	int32_t min;
	int16_t i;

	best = -1;
	min = INT32_C(0x7fffffff);
	for (i = 0; i < TrackCount; i++) {
		if (TrackDone[i] == 0) {
			if (TrackClock[i] < min) {
				best = i;
				min = TrackClock[i];
			}
		}
	}
	if (min != 0)
		SongClock += min * TickLength;
	for (i = 0; i < TrackCount; i++)
		TrackClock[i] -= min;
	return best;
}

/* Play every event that is due; nonzero once the song has ended or stopped. */
int16_t PlayDueEvents(void)
{
	int16_t done;
	int16_t event, value, type, len;
	int16_t track, channel, back;
	int32_t delta;
	int16_t i, j;

	done = 0;
	while (done == 0) {
		track = NextDueTrack();
		if (SongClock > INT32_C(0xffff))
			break;
		event = *TrackPos[track]++;
		if (event >= MIDI_NOTE_OFF && event < MIDI_SYSEX) {
			TrackStatus[track] = event;
			event = *TrackPos[track]++;
		}
		if (event < MIDI_SYSEX) {
			channel = (TrackStatus[track] & 0xf) + 1;
			if (ChannelOwner[channel] == OWNER_NONE || ChannelPriority[channel] < 0) {
				DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
				DriverControlEntry(channel, CONTROL_RESET, 0);
				if (ChannelProgram[channel] != SongProgram[channel]) {
					if (channel != PERCUSSION_CHANNEL)
						DriverProgramEntry(channel, SongProgram[channel]);
					ChannelProgram[channel] = SongProgram[channel];
				}
				DriverControlEntry(channel, CONTROL_VOLUME, ChannelVolume[channel] * SongVolume >> 8);
				DriverControlEntry(channel, CONTROL_PAN, ChannelPan[channel]);
				ChannelOwner[channel] = OWNER_SONG;
			}
			if (ChannelOwner[channel] == OWNER_SONG)
				ClaimMidiChannel(channel, 0);
			switch (TrackStatus[track] & 0xf0) {
			case MIDI_NOTE_OFF:
				value = *TrackPos[track]++;
				if (MusicOn != 0 && ChannelOwner[channel] == OWNER_SONG) {
					if (channel == PERCUSSION_CHANNEL && MusicDevice == MUSIC_DEVICE_ADLIB)
						DriverNoteEntry(PercussionChannel[event], PercussionNote[event], 0);
					else
						DriverNoteEntry(channel, event, 0);
				}
				break;
			case MIDI_NOTE_ON:
				value = *TrackPos[track]++;
				if (MusicOn != 0 && ChannelOwner[channel] == OWNER_SONG) {
					if (channel == PERCUSSION_CHANNEL && MusicDevice == MUSIC_DEVICE_ADLIB)
						DriverNoteEntry(PercussionChannel[event], PercussionNote[event], value);
					else
						DriverNoteEntry(channel, event, value);
				}
				break;
			case MIDI_AFTERTOUCH:
				value = *TrackPos[track]++;
				break;
			case MIDI_CONTROL:
				value = *TrackPos[track]++;
				if (event == CONTROL_EXIT && SongExitPending != 0) {
					MusicFlags |= MUSIC_ENDED;
					for (j = 2; j <= 10; j++) {
						if (MusicOn != 0 && ChannelOwner[j] == OWNER_SONG) {
							DriverControlEntry(j, CONTROL_NOTES_OFF, 0);
							DriverControlEntry(j, CONTROL_RESET, 0);
						}
					}
					return 1;
				}
				if (event == CONTROL_LOOP_START) {
					for (i = 0; i < MAX_TRACKS; i++) {
						LoopTrackPos[i] = TrackPos[i];
						LoopTrackStatus[i] = TrackStatus[i];
						LoopTrackClock[i] = TrackClock[i];
						LoopTrackDone[i] = TrackDone[i];
					}
				} else if (event == CONTROL_LOOP_END) {
					for (i = 0; i < MAX_TRACKS; i++) {
						TrackPos[i] = LoopTrackPos[i];
						TrackStatus[i] = LoopTrackStatus[i];
						TrackClock[i] = LoopTrackClock[i];
						TrackDone[i] = LoopTrackDone[i];
					}
				} else {
					if (MusicOn != 0 && ChannelOwner[channel] == OWNER_SONG) {
						if (event == CONTROL_VOLUME)
							DriverControlEntry(channel, event, value * SongVolume >> 8);
						else
							DriverControlEntry(channel, event, value);
					}
					if (event == CONTROL_VOLUME)
						ChannelVolume[channel] = value;
					if (event == CONTROL_PAN)
						ChannelPan[channel] = value;
				}
				break;
			case MIDI_PROGRAM:
				if (MusicOn != 0 && ChannelOwner[channel] == OWNER_SONG) {
					DriverProgramEntry(channel, event);
					ChannelProgram[channel] = event;
				}
				SongProgram[channel] = event;
				break;
			case MIDI_PITCH_BEND:
				value = *TrackPos[track]++;
				if (MusicOn != 0 && ChannelOwner[channel] == OWNER_SONG)
					DriverPitchEntry(channel, event, value);
				break;
			}
		} else {
			switch (event) {
			case MIDI_SYSEX:
			case MIDI_SYSEX_END:
				len = ReadVarLength(track);
				TrackPos[track] += len;
				break;
			case MIDI_SONG_EVENT:
				channel = (TrackStatus[track] & 0xf) + 1;
				type = *TrackPos[track]++;
				len = *TrackPos[track]++;
				if (type == SONG_LOOP) {
					if (SongExitPending != 0)
						TrackPos[track] += len;
					else {
						MusicFlags |= MUSIC_ENDED;
						DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
						DriverControlEntry(channel, CONTROL_RESET, 0);
						back = TrackPos[track][0] + (TrackPos[track][1] << 8);
						TrackPos[track] -= back;
					}
				} else if (type == SONG_EXIT) {
					if (SongExitPending != 0) {
						MusicFlags |= MUSIC_ENDED;
						for (j = 2; j <= 10; j++) {
							if (MusicOn != 0 && ChannelOwner[j] == OWNER_SONG) {
								DriverControlEntry(j, CONTROL_NOTES_OFF, 0);
								DriverControlEntry(j, CONTROL_RESET, 0);
							}
						}
						return 1;
					}
				} else if (type == SONG_EXIT_LOOP) {
					if (SongExitPending != 0) {
						MusicFlags |= MUSIC_ENDED;
						DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
						DriverControlEntry(channel, CONTROL_RESET, 0);
						back = TrackPos[track][0] + (TrackPos[track][1] << 8);
						TrackPos[track] -= back;
					} else
						TrackPos[track] += len;
				} else if (type == SONG_MARKER) {
					SongMarker = TrackPos[track][0];
					TrackPos[track] += len;
				} else if (type == SONG_BRANCH_UNLESS) {
					if (TrackPos[track][2] != SongBranchValue) {
						MusicFlags |= MUSIC_ENDED;
						DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
						DriverControlEntry(channel, CONTROL_RESET, 0);
						back = TrackPos[track][0] + (TrackPos[track][1] << 8);
						TrackPos[track] -= back;
					} else
						TrackPos[track] += len;
				} else if (type == SONG_BRANCH_IF) {
					if (TrackPos[track][2] == SongBranchValue) {
						MusicFlags |= MUSIC_ENDED;
						DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
						DriverControlEntry(channel, CONTROL_RESET, 0);
						back = TrackPos[track][0] + (TrackPos[track][1] << 8);
						TrackPos[track] -= back;
					} else
						TrackPos[track] += len;
				} else
					TrackPos[track] += len;
				break;
			case MIDI_META:
				type = *TrackPos[track]++;
				len = ReadVarLength(track);
				HandleMetaEvent(track, type, len);
				if (type == META_END_OF_TRACK) {
					TrackDone[track] = 1;
					for (j = 0; j < TrackCount; j++)
						if (TrackDone[j] == 0)
							break;
					if (j == TrackCount)
						done = 1;
				}
				break;
			}
		}
		if (done == 0) {
			delta = ReadVarLength(track);
			AddTrackDelta(track, delta);
		}
	}
	return done;
}

/* Stop the song. */
void StopSong(void)
{
	int16_t i;

	SongStopped = 1;
	CurrentSong = 0;
	MusicFlags |= MUSIC_ENDED;
	for (i = 1; i <= 16; i++) {
		/* Past the table, DOS read the bytes of TickLength, which followed it. */
		uint8_t owner = i < CHANNEL_LINKS ? ChannelOwner[i] : (uint8_t) (TickLength >> (8 * (i - CHANNEL_LINKS)));

		if (owner == OWNER_SONG) {
			DriverControlEntry(i, CONTROL_NOTES_OFF, 0);
			DriverControlEntry(i, CONTROL_RESET, 0);
		}
	}
}

/* Set the music volume of every music channel. */
void ApplySongVolume(int16_t volume)
{
	int16_t i;

	SongVolume = volume;
	for (i = 2; i <= 10; i++) {
		if (ChannelOwner[i] == OWNER_SONG)
			DriverControlEntry(i, CONTROL_VOLUME, ChannelVolume[i] * SongVolume >> 8);
	}
}

/* The timer: fade, play what is due, start the queued song, tick the effects. */
void MusicTimerHandler(void)
{
	uint8_t *song;

	if (MusicDevice != 0) {
		song = CurrentSong;
		if (FadeStep != 0 && SongStopped == 0) {
			FadeLevel += FadeStep;
			if (FadeLevel < 0)
				FadeStep = 0;
			else
				ApplySongVolume(MusicVolume * (FadeLevel >> 8) >> 7);
			if (FadeStep == 0) {
				if (FadeKeepsSong == 0)
					StopSong();
				ApplySongVolume(MusicVolume);
			}
		}
		if (SongStopped == 0) {
			SongStopped = PlayDueEvents();
			if (SongStopped != 0) {
				MusicFlags |= MUSIC_ENDED;
				CurrentSong = 0;
			}
			SongClock -= INT32_C(0x10000);
		}
		if (SongStopped != 0 && QueuedSong != 0) {
			PlaySong(QueuedSong);
			if (MusicFlags & MUSIC_RETURN) {
				QueuedSong = song;
				MusicFlags &= ~MUSIC_RETURN;
			}
		}
		TickMidiSfx();
	}
}

/* The 60 Hz sound tick, run with the sound lock held, in the order the timer chain ran. */
static void SoundTick(void)
{
	if (SoundTickFirst != 0)
		SoundTickFirst();
	if (DriverTimerEntry != 0)
		DriverTimerEntry();
	MusicTimerHandler();
}

/* Rewind every track to the given one, or to all when -1. */
void RewindSong(int16_t only)
{
	int16_t i;

	for (i = 2; i <= 10; i++)
		DriverControlEntry(i, CONTROL_RESET, 0);
	for (i = 0; i < TrackCount; i++) {
		TrackClock[i] = 0;
		if (only == -1 || only == i)
			TrackDone[i] = 0;
		else
			TrackDone[i] = 1;
		TrackPos[i] += 8;
		AddTrackDelta(i, ReadVarLength(i));
	}
	SongStopped = SongExitPending = SongMarker = 0;
	SongFromStart = 1;
	MusicFlags &= ~MUSIC_ENDED;
	MusicFlags |= MUSIC_STARTED;
	QueuedSong = 0;
}

/* Fade the song out over ticks, or stop it at once. */
void FadeOutSong(uint16_t ticks)
{
	SoundLock hold;

	MusicFlags &= ~MUSIC_STARTED;
	if (MusicDevice != 0) {
		if (FadeStep == 0)
			FadeLevel = 0x7fff;
		QueuedSong = 0;
		if (ticks != 0) {
			FadeStep = -(0x8000 / ticks);
			FadeKeepsSong = 0;
		} else {
			FadeStep = 0;
			StopSong();
			ApplySongVolume(MusicVolume);
		}
	}
}

/* Read record n of the named FLX file into a new far block. */
void * ReadFlexRecord(char *name, int16_t n, int16_t flags)
{
	void *block = 0;
	Flex file;
	FlexEntry where;
	int32_t size;

	if (file.open(name)) {
		file.getEntry(n, &where);
		size = where.size;
		block = AllocateFarHeap(size, flags);
		if (block != 0)
			file.readRecord(n, block, 0);
		file.close();
	}
	return block;
}

/* Load the timbre bank and start the driver; only the MT-32 has one. */
int16_t LoadSoundDriver(char *timbres, char *)
{
	int16_t n;

	if (MusicDevice != 0) {
		n = MusicDevice - 1;
		TimbreBank = ReadFlexRecord(timbres, n, FAR_RETAINED);
		if (TimbreBank == 0 && MusicDevice != MUSIC_DEVICE_MT32)
			MusicDevice = 0;
		if (MusicDevice == MUSIC_DEVICE_MT32) {
			DriverDescribeEntry = Mt32Describe;
			DriverInitEntry = Mt32Init;
			DriverShutdownEntry = Mt32Shutdown;
			DriverTimerEntry = Mt32Timer;
			DriverNoteEntry = Mt32Note;
			DriverControlEntry = Mt32Control;
			DriverPitchEntry = Mt32Pitch;
			DriverProgramEntry = Mt32Program;
			DriverPortEntry = Mt32Port;
			DriverSysexEntry = Mt32Sysex;
			DriverReleaseChannelEntry = 0;
			DriverClaimChannelEntry = 0;
			SoundDriverInfo = DriverDescribeEntry();
			if (DriverInitEntry(TimbreBank) == 0) {
				FreeFarHeap(TimbreBank);
				SoundDriverInfo = 0;
				MusicDevice = 0;
				return 0;
			}
			if (!SoundDriverInfo->keepTimbres)
				FreeFarHeap(TimbreBank);
			return 1;
		}
		if (TimbreBank != 0)
			FreeFarHeap(TimbreBank);
	}
	return 0;
}

/* Start the sound driver and the sound tick. */
void StartSoundDriver(char *timbres, char *driver)
{
	uint8_t volume;

	if (MusicDevice != 0) {
		if (!LoadSoundDriver(timbres, driver)) {
			MusicDevice = 0;
			return;
		}
		if (MusicDevice == MUSIC_DEVICE_ADLIB) {
			DriverProgramEntry(10, 0x80);
			DriverProgramEntry(11, 0x72);
			DriverProgramEntry(12, 0x83);
			DriverProgramEntry(13, 0x71);
			DriverProgramEntry(14, 0x86);
			DriverProgramEntry(15, 0x87);
			DriverProgramEntry(16, 0x85);
			DriverProgramEntry(17, 0x9c);
			DriverProgramEntry(18, 0x93);
			DriverProgramEntry(19, 0x9e);
			DriverProgramEntry(20, 0x8d);
			DriverProgramEntry(21, 0x8f);
			DriverProgramEntry(22, 0x90);
			DriverProgramEntry(23, 0x91);
			DriverProgramEntry(24, 0x93);
			DriverProgramEntry(25, 0x8c);
			DriverProgramEntry(26, 0x8b);
			DriverProgramEntry(27, 0x84);
			DriverProgramEntry(28, 0x8a);
			DriverProgramEntry(29, 0x81);
			DriverProgramEntry(30, 0x88);
		}
		if (MusicDevice == MUSIC_DEVICE_MT32) {
			volume = 70;
			DriverSysexEntry(INT32_C(0x100016), 1, &volume);
		}
		SongStopped = 1;
		CurrentSong = 0;
		QueuedSong = 0;
		MusicFlags = 0;
		plat_sound_lock();
		plat_sound_tick_set(SoundTick);
		plat_sound_unlock();
		MidiSfxReady = 1;
	}
}

/* Silence everything. */
void StopSoundDriver(void)
{
	int16_t i;
	SoundLock hold;

	if (MusicDevice != 0) {
		MidiSfxReady = 0;
		SongStopped = 1;
		CurrentSong = 0;
		QueuedSong = 0;
		MusicFlags = 0;
		for (i = 0; i < SFX_VOICES; i++)
			MidiSfxVoices[i].channel = 0;
		for (i = 2; i < 10; i++)
			ReturnChannelToSong(i);
		for (i = 1; i <= 16; i++) {
			DriverControlEntry(i, CONTROL_NOTES_OFF, 0);
			DriverControlEntry(i, CONTROL_RESET, 0);
		}
		DriverShutdownEntry();
	}
}

/* Silence everything. */
void SilenceAllSound(void)
{
	StopSoundDriver();
}

/* Find the MIDI header and the start of each track. */
int16_t OpenSong(uint8_t *song)
{
	int16_t ok = 1;
	int16_t pos;
	int16_t i;

	SongStopped = 1;
	CurrentSong = song;
	if (song == 0) {
		ok = 0;
	} else {
		for (pos = 0; !MatchChunkId(song + pos, "MThd"); pos += GetChunkLength(song + pos) + 8) {
			if (MatchChunkId(song + pos, "MTrk")) {
				ok = 0;
				break;
			}
		}
		if (GetChunkLength(song + pos) != 6)
			ok = 0;
		if (ok) {
			SongFormat = (song[pos + 8] << 8) + song[pos + 9];
			TrackCount = (song[pos + 10] << 8) + song[pos + 11];
			TicksPerBeat = (song[pos + 12] << 8) + song[pos + 13];
			if (SongFormat > 2 || SongFormat < 0)
				ok = 0;
			if (TrackCount > MAX_TRACKS || TrackCount < 0)
				ok = 0;
		}
		if (ok) {
			for (i = 0; i < TrackCount; i++) {
				while (!MatchChunkId(&song[pos + 14], "MTrk"))
					pos += GetChunkLength(&song[pos + 14]) + 8;
				TrackPos[i] = &song[pos + 14];
				pos += GetChunkLength(&song[pos + 14]) + 8;
			}
			RewindSong(-1);
		}
	}
	return ok;
}

/* Play a song at once. */
int16_t PlaySong(uint8_t *song)
{
	SoundLock hold;

	if (MusicDevice == 0)
		return 0;
	FadeOutSong(0);
	return OpenSong(song);
}

/* Play a song, or fade the current one out over fade ticks and queue the song after it. */
void ChangeSong(uint8_t *song, uint16_t fade, int16_t flags)
{
	uint8_t *next;
	SoundLock hold;

	MusicFlags = flags;
	if (MusicDevice != 0) {
		if (FadeStep == 0)
			FadeLevel = 0x7fff;
		if (fade != 0) {
			FadeStep = -(0x8000 / fade);
			QueuedSong = song;
			FadeKeepsSong = 0;
			SongFromStart = 0;
		} else {
			FadeStep = 0;
			QueuedSong = 0;
			if (MusicFlags & MUSIC_RETURN) {
				next = CurrentSong;
				MusicFlags &= ~MUSIC_RETURN;
			} else
				next = 0;
			OpenSong(song);
			QueuedSong = next;
		}
	}
}

/* Queue a song to follow the current one. */
void QueueSong(uint8_t *song, int16_t flags)
{
	SoundLock hold;

	QueuedSong = song;
	SongExitPending = 1;
	SongFromStart = 0;
	MusicFlags = flags;
}

/* Wait until the player raises any of the flags in mask, then clear them. */
void WaitForMusicFlags(uint16_t mask)
{
	if (MusicDevice != 0) {
		plat_sound_lock();
		while (!(MusicFlags & mask)) {
			plat_sound_unlock();
			plat_yield();
			plat_sound_lock();
		}
		MusicFlags &= ~mask;
		plat_sound_unlock();
	}
}

/* Set the music volume. */
void SetMusicVolume(int16_t volume)
{
	int16_t i;
	SoundLock hold;

	if (MusicDevice != 0) {
		MusicVolume = volume;
		if (FadeStep == 0)
			SongVolume = volume;
		if (SongStopped == 0) {
			for (i = 2; i <= 10; i++) {
				if (ChannelOwner[i] == OWNER_SONG)
					DriverControlEntry(i, CONTROL_VOLUME, (ChannelVolume[i] * volume) >> 8);
			}
		}
	}
}

/* Switch the music on or off. */
void EnableMusic(int16_t on)
{
	int16_t i;
	SoundLock hold;

	if (MusicDevice != 0 && (on != 0) != MusicOn) {
		MusicOn = on;
		if (MusicOn != 0) {
			for (i = 2; i <= 10; i++) {
				if (ChannelOwner[i] == OWNER_SONG) {
					if (i != PERCUSSION_CHANNEL)
						DriverProgramEntry(i, SongProgram[i]);
					ChannelProgram[i] = SongProgram[i];
					DriverControlEntry(i, CONTROL_VOLUME, ChannelVolume[i] * SongVolume >> 8);
					DriverControlEntry(i, CONTROL_PAN, ChannelPan[i]);
				}
			}
		} else {
			for (i = 2; i <= 10; i++) {
				if (ChannelOwner[i] == OWNER_SONG) {
					DriverControlEntry(i, CONTROL_NOTES_OFF, 0);
					DriverControlEntry(i, CONTROL_RESET, 0);
				}
			}
		}
	}
}

}

extern "C" void ResetMidiplayGlobals(void)
{
	using namespace Shared;

	SoundDriverInfo = 0;
	DriverDescribeEntry = 0;
	DriverInitEntry = 0;
	DriverShutdownEntry = 0;
	DriverTimerEntry = 0;
	DriverNoteEntry = 0;
	DriverControlEntry = 0;
	DriverPitchEntry = 0;
	DriverProgramEntry = 0;
	DriverPortEntry = 0;
	DriverSysexEntry = 0;
	DriverReleaseChannelEntry = 0;
	DriverClaimChannelEntry = 0;
	SoundTickFirst = 0;
	TimbreBank = 0;
	memset(unused_global_7, 0, sizeof unused_global_7);
	memset(LoopTrackPos, 0, sizeof LoopTrackPos);
	memset(LoopTrackStatus, 0, sizeof LoopTrackStatus);
	memset(LoopTrackClock, 0, sizeof LoopTrackClock);
	memset(LoopTrackDone, 0, sizeof LoopTrackDone);
	memset(TrackPos, 0, sizeof TrackPos);
	memset(TrackStatus, 0, sizeof TrackStatus);
	memset(TrackClock, 0, sizeof TrackClock);
	memset(TrackDone, 0, sizeof TrackDone);
	SongFormat = 0;
	TrackCount = 0;
	TicksPerBeat = 0;
	MusicDevice = 0;
	MidiSfxReady = 0;
	SongMarker = 0;
	SongBranchValue = 0;
	SongFromStart = 1;
	MusicOn = 1;
	SongVolume = 256;
	MusicVolume = 256;
	FadeStep = 0;
	FadeLevel = 0;
	FadeKeepsSong = 0;
	SongExitPending = 0;
	QueuedSong = 0;
	CurrentSong = 0;
	memset(MidiSfxVoices, 0, sizeof MidiSfxVoices);
	memset(ChannelProgram, 255, sizeof ChannelProgram);
	memset(ChannelVolume, 127, sizeof ChannelVolume);
	memset(ChannelPan, 64, sizeof ChannelPan);
	memset(SongProgram, 0, sizeof SongProgram);
	memcpy(ChannelLinks, ChannelLinksStart, sizeof ChannelLinks);
	memset(ChannelPriority, 0, sizeof ChannelPriority);
	memset(ChannelOwner, 0, sizeof ChannelOwner);
	TickLength = 10000;
	SongClock = 0;
	SongStopped = 1;
	MusicFlags = 0;
}
