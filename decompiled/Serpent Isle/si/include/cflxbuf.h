#ifndef CFLXBUF_H
#define CFLXBUF_H

#include "flex.h"

/* One speech entry of a Flex file, read a piece at a time. */
struct FlexSpeechCache {
	Flex archive;
	int entryIndex;
	FlexEntry entryInfo;
	long remaining;
	long entrySize;
	FlexSpeechCache();
	FlexSpeechCache(char);
	~FlexSpeechCache();
	virtual char isOpen();
	virtual unsigned char open(char *name);
	virtual unsigned long fileSize();
	virtual long read(void far *to, unsigned long count);
	virtual void close();
	void playEntry(char *name, int entry, unsigned blocks);
};

/* A speech entry loaded whole into memory borrowed from the shape cache, given back by size. */
struct BorrowedSpeechCache : FlexSpeechCache {
	long bufferSize;
	long buffer;
	long pos;
	BorrowedSpeechCache();
	long read(void far *to, unsigned long count);
	virtual unsigned char allocateBuffer(unsigned long size);
	virtual void freeBuffer();
	void load();
	void skipCreativeHeader();
};

#endif
