#ifndef SHARED_CFLXBUF_H
#define SHARED_CFLXBUF_H

#include "flex.h"

namespace Shared {

/* Sound read from a file into a buffer and handed to the speech card a block at a time. */
struct SpeechCache {
	uint32_t size;         /* bytes allocated for the buffer */
	uint32_t length;       /* bytes of data in the buffer */
	uint32_t pos;          /* offset of the current block */
	int32_t unusedField1;
	uint16_t block;
	uint8_t started;
	int8_t primed;                /* a block is ready */
	int16_t rate;
	int32_t buffer;
	int8_t streaming;             /* refill the buffer block by block; 0 plays a file loaded whole */
	SpeechCache();
	~SpeechCache();
	virtual int32_t read(void *to, uint32_t n) = 0;
	virtual int8_t isOpen() = 0;
	virtual uint8_t open(char *name) = 0;
	virtual void close() = 0;
	virtual uint32_t fileSize() = 0;
	virtual uint8_t allocateBuffer(uint32_t n);
	virtual void freeBuffer();
	void stop();
	void releaseBuffer();
	void prime();
	void play(char *name, uint16_t blocks);
	void playWhole(char *name);
	int8_t checkCreativeHeader(int32_t);
	void queueNextBlock();
	void fillDoubleBuffer(void *to, uint16_t *firstSize, uint16_t *secondSize);
	uint16_t readBytes(void *to, uint16_t n);
	uint8_t isPrimed() { return primed; }
	int16_t getRate() { return rate; }
	int8_t isStreaming() { return streaming; }
};

struct FlexSpeechCache : SpeechCache {
	Flex archive;
	int16_t entryIndex;
	FlexEntry entryInfo;
	int32_t remaining;
	int32_t entrySize;
	FlexSpeechCache();
	FlexSpeechCache(int8_t);
	~FlexSpeechCache();
	int32_t read(void *, uint32_t);
	int8_t isOpen();
	uint8_t open(char *name);
	void close();
	void playEntry(char *name, int16_t entry, uint16_t blocks);
	uint32_t fileSize();
};

/* A flex speech cache whose buffer is borrowed from the shape cache and given back by size. */
struct BorrowedSpeechCache : FlexSpeechCache {
	int32_t bufferSize;
	BorrowedSpeechCache();
	uint8_t allocateBuffer(uint32_t);
	void freeBuffer();
};

}

#endif
