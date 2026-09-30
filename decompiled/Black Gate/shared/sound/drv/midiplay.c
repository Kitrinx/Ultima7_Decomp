/* Black Gate MAINMENU.EXE, resident segment 45 (file offsets 0x01295b to 0x014668, 7437 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "flex.h"
#include "systimer.h"
#include "memapi.h"
#include "midiplay.h"
#include "init.h"

/*
 * The MIDI player: sound effects on borrowed channels, and a Standard MIDI File
 * song played from the timer interrupt through a loaded sound driver.
 *
 *   game --> start/stop/control effect --> voices[32] --\
 *   game --> play/queue/fade song -------> tracks[32] ---+--> driver entry points
 *   timer interrupt --> events due --> tracks ----------/
 */

#define MAKE_FAR_PTR(seg, off) ((void far *)(((unsigned long)(seg) << 16) | (unsigned)(off)))

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

typedef void interrupt (far *Handler)(...);
typedef void (far *Routine)();

/* One sound effect: a list of 8-byte notes played on one channel, or two for stereo. */
struct Voice {
	unsigned char flags;
	unsigned char channel;
	unsigned char channel2;
	unsigned char note;
	unsigned char volume;
	unsigned char pan;
	int id;
	int duration;
	unsigned char far *data;
};

/* What the driver's entry returns. */
struct SoundDriver {
	char unusedField1;
	unsigned char keepTimbres;
	long oldTimer;
	char unusedField2[4];
	long (far *hookInterrupt)(int, InterruptHandler, HookRecord *, long far *);
	void (far *delay)(unsigned long);
	int first;
};

unsigned far *SoundDriverImage;
SoundDriver far *SoundDriverInfo;
SoundDriver far *(far *DriverDescribeEntry)(void);
int (far pascal *DriverInitEntry)(void far *);
void (far *DriverShutdownEntry)(void);
Routine DriverTimerEntry;
void (far pascal *DriverNoteEntry)(int, int, int);
void (far pascal *DriverControlEntry)(int, int, int);
void (far pascal *DriverPitchEntry)(int, int, int);
void (far pascal *DriverProgramEntry)(int, int);
void (far pascal *DriverPortEntry)(int);
void (far pascal *DriverSysexEntry)(long, int, void far *);
void (far *DriverReleaseChannelEntry)(void);
void (far *DriverClaimChannelEntry)(void);
void far *TimbreBank;
char unused_global_7[20];       /* never referenced */
unsigned char far *LoopTrackPos[MAX_TRACKS];
unsigned char LoopTrackStatus[MAX_TRACKS];
long LoopTrackClock[MAX_TRACKS];
int LoopTrackDone[MAX_TRACKS];
unsigned char far *TrackPos[MAX_TRACKS];
unsigned char TrackStatus[MAX_TRACKS];
long TrackClock[MAX_TRACKS];
int TrackDone[MAX_TRACKS];
int SongFormat;
int TrackCount;
int TicksPerBeat;
Handler PrevMusicTimer;

int MusicDevice = 0;
int MidiSfxReady = 0;
int SongMarker = 0;
int SongBranchValue = 0;
int SongFromStart = 1;
int MusicOn = 1;
int SongVolume = 256;
int MusicVolume = 256;
int FadeStep = 0;
int FadeLevel = 0;
int FadeKeepsSong = 0;
int SongExitPending = 0;
unsigned char far *QueuedSong = 0;
unsigned char far *CurrentSong = 0;
Voice MidiSfxVoices[SFX_VOICES] = { 0 };
unsigned char ChannelProgram[11] = { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 };
unsigned char ChannelVolume[11] = { 127, 127, 127, 127, 127, 127, 127, 127, 127, 127, 127 };
unsigned char ChannelPan[11] = { 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64 };
unsigned char SongProgram[11] = { 0 };

/* Channels 2-9 for effects, each linked to the next in its list; the two list heads follow. */
unsigned char ChannelLinks[CHANNEL_LINKS] = { 0, 1, 3, 4, 5, 6, 7, 8, 9, 11, 10, 2, 12 };
char ChannelPriority[CHANNEL_LINKS] = { 0 };
unsigned char ChannelOwner[CHANNEL_LINKS] = { 0 };
unsigned long TickLength = 10000;
long SongClock = 0;

/* On Adlib, percussion notes are played as this channel and note. */
unsigned char PercussionChannel[88] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 12, 0, 0, 0,
	0, 13, 0, 10, 10, 29, 11, 0, 0, 0, 27, 0, 16, 0, 14, 0,
	0, 15, 0, 30, 0, 0, 28, 0, 21, 0, 0, 0, 26, 26, 25, 20,
	20, 29, 29, 21, 21, 22, 0, 30, 30, 24, 17, 20, 17, 17, 18, 17,
	19, 17, 23, 0, 0, 0, 0, 0
};
unsigned char PercussionNote[88] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 0, 0, 0,
	0, 33, 0, 48, 48, 48, 48, 0, 0, 0, 71, 0, 71, 0, 71, 0,
	0, 79, 0, 77, 0, 0, 54, 0, 71, 0, 0, 0, 72, 79, 79, 64,
	58, 65, 66, 89, 84, 48, 0, 71, 72, 36, 86, 96, 90, 92, 66, 96,
	80, 100, 82, 0, 0, 0, 0, 0
};
int SongStopped = 1;
int MusicFlags = 0;

/* Relink channel n in front of whichever entry links to next. */
static void far LinkMidiChannel(int n, int next)
{
	int i;

	for (i = 0; i < CHANNEL_LINKS; i++) {
		if (ChannelLinks[i] == next) {
			ChannelLinks[i] = n;
			ChannelLinks[n] = next;
			break;
		}
	}
}

/* Move channel n to the end of the busy list at the given priority. */
static void far ClaimMidiChannel(int n, int priority)
{
	int i;

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
void far FreeMidiChannel(int n)
{
	int i;

	if (n >= 2 && n <= 9) {
		disable();
		DriverControlEntry(n, CONTROL_NOTES_OFF, 0);
		DriverControlEntry(n, CONTROL_RESET, 0);
		enable();
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
int far AllocateMidiChannel(int priority)
{
	int n, result;

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
void StartSfxNote(int n)
{
	unsigned char far *p;
	int note, velocity, duration;
	int volume, volume2, pan, pan2;
	Voice *v;
	int program;

	if (MusicDevice != 0) {
		v = &MidiSfxVoices[n];
		p = v->data;
		program = p[1];
		if (program != 0 && v->channel != 0) {
			note = p[2];
			velocity = p[3];
			duration = *(int far *)(p + 4);
			if (v->flags & VOICE_STEREO) {
				pan = 0;
				pan2 = 127;
				volume = v->volume * (v->pan < 32 ? 127 : 159 - v->pan) >> 7;
				volume2 = v->volume * (v->pan > 95 ? 127 : v->pan + 32) >> 7;
			} else {
				volume = volume2 = v->volume;
				pan = pan2 = v->pan;
			}
			disable();
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
			enable();
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
	int flags, changed;
	Voice *v;
	int i;

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
						v->duration = *(int far *)(v->data + 4);
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
Voice far *StartMidiSfx(unsigned char far *data, int flags, int volume, int pan, int id, int priority)
{
	Voice far *v;
	int channel = 0, channel2 = 0;
	int free = -1;
	int i;

	if (MusicDevice != 0) {
		for (i = 0; i < SFX_VOICES; i++) {
			if (MidiSfxVoices[i].channel != 0 && MidiSfxVoices[i].id == id) {
				channel = MidiSfxVoices[i].channel;
				channel2 = MidiSfxVoices[i].channel2;
				disable();
				DriverNoteEntry(channel, MidiSfxVoices[i].note, 0);
				if (MidiSfxVoices[i].flags & VOICE_STEREO)
					DriverNoteEntry(channel2, MidiSfxVoices[i].note, 0);
				enable();
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
void SetMidiSfxControl(Voice far *v, int control, int value, int id)
{
	int volume, volume2;

	if (MusicDevice == 0 || v == 0 || (id != 0 && v->id != id) || v->channel == 0)
		return;
	if (control == CONTROL_VOLUME)
		v->volume = value;
	if (control == CONTROL_PAN)
		v->pan = value;
	if ((v->flags & VOICE_STEREO) && control == CONTROL_PAN) {
		volume = v->volume * (v->pan < 32 ? 127 : 159 - v->pan) >> 7;
		volume2 = v->volume * (v->pan > 95 ? 127 : v->pan + 32) >> 7;
		disable();
		if (ChannelOwner[v->channel] == OWNER_SFX)
			DriverControlEntry(v->channel, CONTROL_VOLUME, volume);
		if (ChannelOwner[v->channel2] == OWNER_SFX) {
			DriverControlEntry(v->channel2, CONTROL_VOLUME, volume2);
			enable();
			return;
		}
		enable();
		return;
	}
	disable();
	if (ChannelOwner[v->channel] == OWNER_SFX)
		DriverControlEntry(v->channel, control, value);
	if ((v->flags & VOICE_STEREO) && ChannelOwner[v->channel2] == OWNER_SFX)
		DriverControlEntry(v->channel2, control, value);
	enable();
}

/* Give a channel back to the music, restoring its program, volume and pan. */
void ReturnChannelToSong(int channel)
{
	if (MusicDevice != 0 && ChannelOwner[channel] == OWNER_SFX) {
		FreeMidiChannel(channel);
		disable();
		DriverControlEntry(channel, CONTROL_NOTES_OFF, 0);
		DriverControlEntry(channel, CONTROL_RESET, 0);
		DriverProgramEntry(channel, SongProgram[channel]);
		ChannelProgram[channel] = SongProgram[channel];
		DriverControlEntry(channel, CONTROL_VOLUME, ChannelVolume[channel] * SongVolume >> 8);
		DriverControlEntry(channel, CONTROL_PAN, ChannelPan[channel]);
		ChannelOwner[channel] = OWNER_SONG;
		enable();
	}
}

/* Stop a sound effect and return its channels to the music. */
void StopMidiSfxVoice(Voice far *v, int id)
{
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
void StopMidiSfx(Voice far *v, int id)
{
	int i;

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
	int i;

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
long far ReadVarLength(int track)
{
	long v = 0;

	while (*TrackPos[track] & 0x80)
		v = (v << 7) + (*TrackPos[track]++ & 0x7f);
	v = (v << 7) + *TrackPos[track]++;
	return v;
}

/* Whether the chunk at p starts with the four-character id. */
int MatchChunkId(unsigned char far *p, char *id)
{
	int i;

	for (i = 0; i < 4; i++)
		if (*p++ != *id++)
			break;
	return i == 4;
}

/* The chunk's length, stored big-endian after its id. */
int GetChunkLength(unsigned char far *p)
{
	long len;

	len = (p[4] << 24) + (p[5] << 16) + (p[6] << 8) + p[7];
	return len;
}

/* A meta event: take up a tempo change, skip the rest. */
void HandleMetaEvent(int track, int type, int len)
{
	long tempo;
	int i;

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
void AddTrackDelta(int track, long delta)
{
	TrackClock[track] += delta;
}

/* The unfinished track whose next event comes soonest; the time until then passes on every track. */
int far NextDueTrack(void)
{
	int best;
	long min;
	int i;

	best = -1;
	min = 0x7fffffffL;
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
int PlayDueEvents(void)
{
	int done;
	int event, value, type, len;
	int track, channel, back;
	long delta;
	int i, j;

	done = 0;
	while (done == 0) {
		track = NextDueTrack();
		if (SongClock > 0xffffL)
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
	int i;

	SongStopped = 1;
	CurrentSong = 0;
	MusicFlags |= MUSIC_ENDED;
	for (i = 1; i <= 16; i++) {
		if (ChannelOwner[i] == OWNER_SONG) {
			DriverControlEntry(i, CONTROL_NOTES_OFF, 0);
			DriverControlEntry(i, CONTROL_RESET, 0);
		}
	}
}

/* Set the music volume of every music channel. */
void ApplySongVolume(int volume)
{
	int i;

	SongVolume = volume;
	for (i = 2; i <= 10; i++) {
		if (ChannelOwner[i] == OWNER_SONG)
			DriverControlEntry(i, CONTROL_VOLUME, ChannelVolume[i] * SongVolume >> 8);
	}
}

/* The timer: fade, play what is due, start the queued song, tick the effects. */
void interrupt MusicTimerHandler(...)
{
	unsigned char far *song;

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
			SongClock -= 0x10000L;
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
	PrevMusicTimer();
}

/* Rewind every track to the given one, or to all when -1. */
void RewindSong(int only)
{
	int i;

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
void FadeOutSong(unsigned ticks)
{
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
void far *far ReadFlexRecord(char *name, int n, int flags)
{
	void far *block = 0;
	FlexEntry where;
	long size;
	Flex file;

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

/* Load the sound driver and its timbre bank and hand the driver its services. */
int LoadSoundDriver(char *timbres, char *driver)
{
	unsigned seg;
	int n;

	if (MusicDevice != 0) {
		n = MusicDevice - 1;
		SoundDriverImage = (unsigned far *)ReadFlexRecord(driver, n, FAR_RETAINED | FAR_ALIGN_PARA);
		TimbreBank = ReadFlexRecord(timbres, n, FAR_RETAINED);
		if (TimbreBank == 0 && MusicDevice != MUSIC_DEVICE_MT32) {
			FreeFarHeap(SoundDriverImage);
			SoundDriverImage = 0;
			MusicDevice = 0;
		}
		if (SoundDriverImage != 0) {
			seg = FP_SEG(SoundDriverImage);
			DriverDescribeEntry = (SoundDriver far *(far *)(void))MAKE_FAR_PTR(seg, SoundDriverImage[1]);
			DriverInitEntry = (int (far pascal *)(void far *))MAKE_FAR_PTR(seg, SoundDriverImage[2]);
			DriverShutdownEntry = (void (far *)(void))MAKE_FAR_PTR(seg, SoundDriverImage[3]);
			DriverTimerEntry = (Routine)MAKE_FAR_PTR(seg, SoundDriverImage[4]);
			DriverNoteEntry = (void (far pascal *)(int, int, int))MAKE_FAR_PTR(seg, SoundDriverImage[5]);
			DriverControlEntry = (void (far pascal *)(int, int, int))MAKE_FAR_PTR(seg, SoundDriverImage[6]);
			DriverPitchEntry = (void (far pascal *)(int, int, int))MAKE_FAR_PTR(seg, SoundDriverImage[7]);
			DriverProgramEntry = (void (far pascal *)(int, int))MAKE_FAR_PTR(seg, SoundDriverImage[8]);
			DriverPortEntry = (void (far pascal *)(int))MAKE_FAR_PTR(seg, SoundDriverImage[9]);
			DriverSysexEntry = (void (far pascal *)(long, int, void far *))MAKE_FAR_PTR(seg, SoundDriverImage[10]);
			DriverReleaseChannelEntry = (void (far *)(void))MAKE_FAR_PTR(seg, SoundDriverImage[11]);
			DriverClaimChannelEntry = (void (far *)(void))MAKE_FAR_PTR(seg, SoundDriverImage[12]);
			SoundDriverInfo = DriverDescribeEntry();
			SoundDriverInfo->hookInterrupt = HookInterruptVector;
			SoundDriverInfo->delay = WaitTicks;
			if (DriverInitEntry(TimbreBank) == 0) {
				FreeFarHeap(SoundDriverImage);
				FreeFarHeap(TimbreBank);
				SoundDriverImage = 0;
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

/* Start the sound driver and hook the timer. */
void StartSoundDriver(char *timbres, char *driver)
{
	unsigned char volume;

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
			DriverSysexEntry(0x100016L, 1, &volume);
		}
		SongStopped = 1;
		CurrentSong = 0;
		QueuedSong = 0;
		MusicFlags = 0;
		disable();
		HookInterruptVector(8, (InterruptHandler)MusicTimerHandler, 0, (long far *)&PrevMusicTimer);
		HookInterruptVector(8, (InterruptHandler)DriverTimerEntry, 0, &SoundDriverInfo->oldTimer);
		enable();
		MidiSfxReady = 1;
	}
}

/* Silence everything. */
void StopSoundDriver(void)
{
	int i;

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
int far OpenSong(unsigned char far *song)
{
	int ok = 1;
	int pos;
	int i;

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
int PlaySong(unsigned char far *song)
{
	if (MusicDevice == 0)
		return 0;
	FadeOutSong(0);
	return OpenSong(song);
}

/* Play a song, or fade the current one out over fade ticks and queue the song after it. */
void ChangeSong(unsigned char far *song, unsigned fade, int flags)
{
	unsigned char far *next;

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
void QueueSong(unsigned char far *song, int flags)
{
	QueuedSong = song;
	SongExitPending = 1;
	SongFromStart = 0;
	MusicFlags = flags;
}

/* Wait until the player raises any of the flags in mask, then clear them. */
void WaitForMusicFlags(unsigned mask)
{
	if (MusicDevice != 0) {
		while (!(MusicFlags & mask))
			;
		MusicFlags &= ~mask;
	}
}

/* Set the music volume. */
void far SetMusicVolume(int volume)
{
	int i;

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
void EnableMusic(int on)
{
	int i;

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
