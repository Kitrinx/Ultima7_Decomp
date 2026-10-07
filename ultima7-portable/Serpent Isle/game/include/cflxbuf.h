#ifndef CFLXBUF_H
#define CFLXBUF_H

#include "flex.h"

/* One speech entry of a Flex file, read a piece at a time. */
struct FlexSpeechCache {
	Flex archive;
	int16_t entryIndex;
	FlexEntry entryInfo;
	int32_t remaining;
	int32_t entrySize;
	FlexSpeechCache();
	FlexSpeechCache(int8_t);
	~FlexSpeechCache();
	virtual int8_t isOpen();
	virtual uint8_t open(char *name);
	virtual uint32_t fileSize();
	virtual int32_t read(void *to, uint32_t count);
	virtual void close();
	void playEntry(char *name, int16_t entry, uint16_t blocks);
};

/* A speech entry loaded whole into memory borrowed from the shape cache, given back by size. */
struct BorrowedSpeechCache : FlexSpeechCache {
	int32_t bufferSize;
	int32_t buffer;
	int32_t pos;
	BorrowedSpeechCache();
	int32_t read(void *to, uint32_t count);
	virtual uint8_t allocateBuffer(uint32_t size);
	virtual void freeBuffer();
	void load();
	void skipCreativeHeader();
};

#endif
