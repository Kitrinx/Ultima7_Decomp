#ifndef SPEECH_H
#define SPEECH_H

#include "digital.h"

namespace Endgame {

/* A sample in linear memory, played through AIL's two sound buffers and refilled as each empties. */
struct StreamSound : DigitalSound {
	uint8_t active;
	uint16_t bufferSize;
	uint8_t *second;
	sound_buff buffer;
	const uint8_t *memory;      /* linear addresses are offsets into this */
	int32_t source;
	int32_t position;
	int32_t length;
	StreamSound(SoundDriver *d, const uint8_t *from, int32_t size, uint16_t buffer) { init(d, from, size, buffer); }
	~StreamSound();
	void init(SoundDriver *d, const uint8_t *from, int32_t size, uint16_t buffer);
	void readLinear(void *to, int32_t size);
	void CopyLinearToFar(void *to, int32_t from, int32_t size) { memcpy(to, memory + from, size); }
	uint8_t PeekByte(int32_t at) { return memory[at]; }
	uint16_t PeekWord(int32_t at) { return (uint16_t) (memory[at] | memory[at + 1] << 8); }
	void service();
	uint8_t isDone();
	void start(uint8_t packType, uint8_t rate);
};

/* A Creative Voice file in linear memory, streamed block by block from a marker on. */
struct SpeechStream : StreamSound {
	uint8_t type;
	int32_t offset;
	int32_t blockEnd;
	int32_t blockSize;
	uint16_t repeats;
	SpeechStream(SoundDriver *d, const uint8_t *from, int32_t size, uint16_t buffer)
		: StreamSound(d, from, size, buffer) {}
	virtual void readLinear(void *to, int32_t size);
	void readHeader();
	uint8_t nextBlock();
	uint8_t isDone();
	void play(int16_t marker);
};

}

#endif
