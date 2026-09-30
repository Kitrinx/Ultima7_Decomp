#ifndef DIGITAL_H
#define DIGITAL_H

#include "sounddrv.h"

extern char *NoSampleMemory;
extern char *NoStreamMemory;
extern char *NoSampleDriver;
extern char *NotDspDriver;

/* A digitized sample played from one AIL sound buffer. */
struct DigitalSound {
	SoundDriver *driver;
	sound_buff sound;
	MemHandle sample;
	DigitalSound() { driver = 0; }
	DigitalSound(SoundDriver *d) { setDriver(d); }
	~DigitalSound() { stop(); }
	void setDriver(SoundDriver *d);
	void load(char *name, int packType, int rate);
	void stop();
	void setAndLoad(SoundDriver *d, char *name, int packType, int rate);
	void play();
	virtual unsigned char isDone();
	virtual unsigned char isPlaying();
	virtual unsigned char isPaused();
	virtual unsigned char isStopped();
	void loadFile(char *name);
	void setData(void far *data);
	void stopPlayback() { if (driver) AIL_stop_digital_playback(driver->getHandle()); }
};

/* A Creative Voice file, played by AIL block by block. */
struct VocSound : DigitalSound {
	VocSound(SoundDriver *d, void far *voc) : DigitalSound(d) { setData(voc); }
	~VocSound() {}
	virtual void play(int block);
	unsigned char isDone();
	unsigned char isPlaying();
	unsigned char isPaused();
	unsigned char isStopped();
};

#endif
