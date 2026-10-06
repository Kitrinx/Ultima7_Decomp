#ifndef XMIDI_H
#define XMIDI_H

#include "sounddrv.h"

extern char *NotXmidiDriver;
extern char *NoSequenceMemory;
extern char *NoSequenceDriver;

/* One sequence of an XMIDI file registered with the driver, with its state table. */
struct XmidiSequence {
	SoundDriver *driver;
	int handle;
	MemHandle state;
	XmidiSequence() { driver = 0; handle = -1; }
	~XmidiSequence() { release(); }
	void release();
	void setDriver(SoundDriver *d);
	void registerSequence(void far *xmidi, int n);
	void start() { if (driver) AIL_start_sequence(driver->getHandle(), handle); }
	void stop() { if (driver) AIL_stop_sequence(driver->getHandle(), handle); }
	void setVolume(unsigned percent, unsigned ms)
		{ if (driver) AIL_set_relative_volume(driver->getHandle(), handle, percent, ms); }
	unsigned char isDone()
	{
		if (driver)
			return AIL_sequence_status(driver->getHandle(), handle) == SEQ_DONE;
		else
			return 1;
	}
	unsigned char isPlaying()
	{
		if (driver)
			return AIL_sequence_status(driver->getHandle(), handle) == SEQ_PLAYING;
		else
			return 0;
	}
};

/* A sequence that holds its own XMIDI file. */
struct XmidiSong : XmidiSequence {
	MemHandle data;
	XmidiSong(SoundDriver *d, char *name, unsigned char timbres) { setAndLoad(d, name, timbres); }
	~XmidiSong() {}
	void load(char *name, unsigned char timbres);
	void setAndLoad(SoundDriver *d, char *name, unsigned char timbres);
	void prepare() { registerSequence(data.pointer(), 0); }
	void play() { prepare(); start(); }
};

/* An XMIDI file in memory and every sequence in it. */
struct XmidiPlayer {
	SoundDriver *driver;
	MemHandle data;
	unsigned count;
	XmidiSequence *sequences;
	XmidiPlayer(SoundDriver *d, char *name, unsigned char timbres) { setAndLoad(d, name, timbres); }
	void setDriver(SoundDriver *d);
	void load(char *name, unsigned char timbres);
	void setAndLoad(SoundDriver *d, char *name, unsigned char timbres);
	void release();
	void play(int n);
	void release(int n);
	void stop(int n);
	void prepare(int n) { if (driver) sequences[n].registerSequence(data.pointer(), n); }
	unsigned char isDone(int n)
	{
		if (driver)
			return sequences[n].isDone();
		else
			return 1;
	}
	unsigned char isPlaying(int n)
	{
		if (driver)
			return sequences[n].isPlaying();
		else
			return 0;
	}
};

#endif
