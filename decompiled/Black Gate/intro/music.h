#ifndef MUSIC_H
#define MUSIC_H

#include "chkfile.h"
#include "midiplay.h"

/* Songs play only while MusicEnabled is set, sound effects while SfxEnabled is. */
extern char MusicEnabled;
extern char SfxEnabled;

inline unsigned char MusicReady() { return MusicDevice != 0; }

/* The music driver: loaded with its timbres on start, silenced on stop. */
struct MusicSystem {
	MusicSystem() {}
	~MusicSystem() { stop(); }
	void start(unsigned char device, char *driverFile, char *timbreFile);
	void stop();
};

/* A song file read whole into the far heap. */
struct Song {
	DataFile file;
	unsigned char far *data;
	Song();
	Song(MusicSystem *music, char *name);
	Song(MusicSystem *music, char *flexName, int entry);
	~Song();
	void load(char *name);
	void load(char *flexName, int entry);
	void play();
	void fadeOut(unsigned ticks);
	unsigned char finished();
	void unload();
};

/* One 8-byte note of a Roland sound effect, as the MIDI player reads it. */
struct SfxNote {
	unsigned char flags;        /* 1 chains to another note, 2 slides, 4 holds, 8 repeats */
	unsigned char patch;        /* program plus one; 0 until its timbre is loaded */
	unsigned char pitch;
	unsigned char velocity;
	int duration;               /* ticks */
	unsigned char slideTo;      /* the pitch a slide ends on */
	unsigned char chain;        /* notes to skip, less one, to reach the chained one */
	SfxNote(unsigned char how, unsigned char program, unsigned char key, unsigned char loudness, int ticks,
		unsigned char slide, unsigned char skip)
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
	int flags, volume, pan, id, priority;
	SoundEffect(SfxNote &n);
	SoundEffect(unsigned char how, unsigned char program, unsigned char key, unsigned char loudness, int ticks,
		unsigned char slide, unsigned char skip);
	void setup(int f, int v, int p, int n, int level);
	void play(int n);
	void stop();
};

void EnableMusic();
void DisableMusic();
void EnableSfx();
void DisableSfx();

#endif
