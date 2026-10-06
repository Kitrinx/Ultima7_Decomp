#ifndef SPEECH_H
#define SPEECH_H

#include "digital.h"

/* A sample in linear memory, played through AIL's two sound buffers and refilled as each empties. */
struct StreamSound : DigitalSound {
	unsigned char active;
	unsigned bufferSize;
	MemHandle second;
	sound_buff buffer;
	long source;
	long position;
	long length;
	StreamSound(SoundDriver *d, long from, long size, unsigned buffer) { init(d, from, size, buffer); }
	void init(SoundDriver *d, long from, long size, unsigned buffer);
	void readLinear(void far *to, long size);
	void service();
	unsigned char isDone();
	void start(unsigned char packType, unsigned char rate);
};

/* A Creative Voice file in linear memory, streamed block by block from a marker on. */
struct SpeechStream : StreamSound {
	unsigned char type;
	long offset;
	long blockEnd;
	long blockSize;
	unsigned repeats;
	SpeechStream(SoundDriver *d, long from, long size, unsigned buffer)
		: StreamSound(d, from, size, buffer) {}
	virtual void readLinear(void far *to, long size);
	void readHeader();
	unsigned char nextBlock();
	unsigned char isDone();
	void play(int marker);
};

#endif
