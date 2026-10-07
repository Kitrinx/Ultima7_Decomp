#ifndef DIGITAL_H
#define DIGITAL_H

#include "sounddrv.h"

namespace Endgame {

extern char *NoSampleMemory;
extern char *NoStreamMemory;
extern char *NoSampleDriver;
extern char *NotDspDriver;

/* A digitized sample played from one AIL sound buffer. */
struct DigitalSound {
	SoundDriver *driver;
	sound_buff sound;
	void *sample;
	DigitalSound() { driver = 0; sample = 0; }
	DigitalSound(SoundDriver *d) { sample = 0; setDriver(d); }
	~DigitalSound() { stop(); }
	void setDriver(SoundDriver *d);
	void load(char *name, int16_t packType, int16_t rate);
	void stop();
	void setAndLoad(SoundDriver *d, char *name, int16_t packType, int16_t rate);
	void play();
	virtual uint8_t isDone();
	virtual uint8_t isPlaying();
	virtual uint8_t isPaused();
	virtual uint8_t isStopped();
	void loadFile(char *name);
	void setData(void *data);
	void stopPlayback() { if (driver) AIL_stop_digital_playback(driver->getHandle()); }
};

/* A Creative Voice file, played by AIL block by block. */
struct VocSound : DigitalSound {
	VocSound(SoundDriver *d, void *voc) : DigitalSound(d) { setData(voc); }
	~VocSound() {}
	virtual void play(int16_t block);
	uint8_t isDone();
	uint8_t isPlaying();
	uint8_t isPaused();
	uint8_t isStopped();
};

}

#endif
