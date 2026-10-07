#ifndef XMIDI_H
#define XMIDI_H

#include "sounddrv.h"

namespace Endgame {

extern char *NotXmidiDriver;
extern char *NoSequenceMemory;
extern char *NoSequenceDriver;

/* One sequence of an XMIDI file registered with the driver, with its state table. */
struct XmidiSequence {
	SoundDriver *driver;
	int16_t handle;
	void *state;
	XmidiSequence() { driver = 0; handle = -1; state = 0; }
	~XmidiSequence() { release(); }
	void release();
	void setDriver(SoundDriver *d);
	void registerSequence(void *xmidi, int16_t n);
	void start() { if (driver) AIL_start_sequence(driver->getHandle(), handle); }
	void stop() { if (driver) AIL_stop_sequence(driver->getHandle(), handle); }
	void setVolume(uint16_t percent, uint16_t ms)
		{ if (driver) AIL_set_relative_volume(driver->getHandle(), handle, percent, ms); }
	uint8_t isDone()
	{
		if (driver)
			return AIL_sequence_status(driver->getHandle(), handle) == SEQ_DONE;
		else
			return 1;
	}
	uint8_t isPlaying()
	{
		if (driver)
			return AIL_sequence_status(driver->getHandle(), handle) == SEQ_PLAYING;
		else
			return 0;
	}
};

/* A sequence that holds its own XMIDI file. */
struct XmidiSong : XmidiSequence {
	void *data;
	XmidiSong(SoundDriver *d, char *name, uint8_t timbres) { data = 0; setAndLoad(d, name, timbres); }
	~XmidiSong() { if (data) FreeFarHeap(data); }
	void load(char *name, uint8_t timbres);
	void setAndLoad(SoundDriver *d, char *name, uint8_t timbres);
	void prepare() { registerSequence(data, 0); }
	void play() { prepare(); start(); }
};

}

#endif
