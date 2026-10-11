#ifndef ENDGAME_XMIDI_H
#define ENDGAME_XMIDI_H

namespace Endgame {

/* An AIL driver with its timbre file: timbres the sequences ask for come from there. */
struct SoundDriver {
	int16_t handle;             /* -1 when the driver found no synth */
	void *image;
	void *timbreCache;
	int16_t timbreFile;
	SoundDriver(const char *driver, const char *timbres);
	~SoundDriver();
	uint8_t isOpen() { return handle != -1; }
	uint8_t *loadTimbre(uint8_t bank, uint8_t patch);
	void installTimbre(uint8_t bank, uint8_t patch);
};

/* An XMIDI file in memory and every sequence in it. */
struct XmidiPlayer {
	SoundDriver *driver;
	uint8_t *data;
	int32_t size;
	uint16_t count;
	int16_t *handles;
	void **states;
	XmidiPlayer(SoundDriver *d, const char *name);
	~XmidiPlayer();
	void play(int16_t n);
	void stop(int16_t n);
	uint8_t isDone(int16_t n);
	uint8_t isPlaying(int16_t n);
};

}

#endif
