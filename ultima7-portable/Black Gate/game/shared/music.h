#ifndef SHARED_MUSIC_H
#define SHARED_MUSIC_H

#include "chkfile.h"
#include "midiplay.h"

namespace Shared {

/* Songs play only while MusicEnabled is set, sound effects while SfxEnabled is. */
extern char MusicEnabled;
extern char SfxEnabled;

inline uint8_t MusicReady() { return MusicDevice != 0; }

/* The music driver: loaded with its timbres on start, silenced on stop. */
struct MusicSystem {
	MusicSystem() {}
	~MusicSystem() { stop(); }
	void start(uint8_t device, char *driverFile, char *timbreFile);
	void stop();
};

/* A song file read whole into the far heap. */
struct Song {
	DataFile file;
	uint8_t *data;
	Song();
	Song(MusicSystem *music, char *name);
	Song(MusicSystem *music, char *flexName, int16_t entry);
	~Song();
	void load(char *name);
	void load(char *flexName, int16_t entry);
	void play();
	void fadeOut(uint16_t ticks);
	uint8_t finished();
	void unload();
};

/* One 8-byte note of a Roland sound effect, as the MIDI player reads it. */
struct SfxNote {
	uint8_t flags;              /* 1 chains to another note, 2 slides, 4 holds, 8 repeats */
	uint8_t patch;              /* program plus one; 0 until its timbre is loaded */
	uint8_t pitch;
	uint8_t velocity;
	int16_t duration;           /* ticks */
	uint8_t slideTo;            /* the pitch a slide ends on */
	uint8_t chain;              /* notes to skip, less one, to reach the chained one */
	SfxNote(uint8_t how, uint8_t program, uint8_t key, uint8_t loudness, int16_t ticks,
		uint8_t slide, uint8_t skip)
	{
		flags = how;
		patch = program;
		pitch = key;
		velocity = loudness;
		duration = ticks;
		slideTo = slide;
		chain = skip;
	}
};

#define SFX_FULL_VOLUME     255
#define SFX_PAN_CENTER      64

/* A note with the settings the MIDI player starts it with. */
struct SoundEffect {
	SfxNote note;
	int16_t flags, volume, pan, id, priority;
	SoundEffect(SfxNote &n);
	SoundEffect(uint8_t how, uint8_t program, uint8_t key, uint8_t loudness, int16_t ticks,
		uint8_t slide, uint8_t skip);
	void setup(int16_t f, int16_t v, int16_t p, int16_t n, int16_t level);
	void play(int16_t n);
	void stop();
};

void EnableMusic();
void DisableMusic();
void EnableSfx();
void DisableSfx();

}

#endif
