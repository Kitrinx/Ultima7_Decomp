#ifndef CFLXBUF_H
#define CFLXBUF_H

#include "flex.h"

/* Sound read from a file into a buffer and handed to the speech card a block at a time. */
struct SpeechCache {
	unsigned long size;         /* bytes allocated for the buffer */
	unsigned long length;       /* bytes of data in the buffer */
	unsigned long pos;          /* offset of the current block */
	long unusedField1;
	unsigned block;
	unsigned char started;
	char primed;                /* a block is ready */
	int rate;
	long buffer;
	char streaming;             /* refill the buffer block by block; 0 plays a file loaded whole */
	SpeechCache();
	~SpeechCache();
	virtual long read(void far *to, unsigned long n) = 0;
	virtual char isOpen() = 0;
	virtual unsigned char open(char *name) = 0;
	virtual void close() = 0;
	virtual unsigned long fileSize() = 0;
	virtual unsigned char allocateBuffer(unsigned long n);
	virtual void freeBuffer();
	void stop();
	void releaseBuffer();
	void prime();
	void play(char *name, unsigned blocks);
	void playWhole(char *name);
	char checkCreativeHeader(long);
	void queueNextBlock();
	void fillDoubleBuffer(void far *to, unsigned *firstSize, unsigned *secondSize);
	unsigned readBytes(void far *to, unsigned n);
};

struct FlexSpeechCache : SpeechCache {
	Flex archive;
	int entryIndex;
	FlexEntry entryInfo;
	long remaining;
	long entrySize;
	FlexSpeechCache();
	FlexSpeechCache(char);
	~FlexSpeechCache();
	long read(void far *, unsigned long);
	char isOpen();
	unsigned char open(char *name);
	void close();
	void playEntry(char *name, int entry, unsigned blocks);
	unsigned long fileSize();
};

/* A flex speech cache whose buffer is borrowed from the shape cache and given back by size. */
struct BorrowedSpeechCache : FlexSpeechCache {
	long bufferSize;
	BorrowedSpeechCache();
	unsigned char allocateBuffer(unsigned long);
	void freeBuffer();
};

#endif
