/* Black Gate U7.EXE, resident segment 48 (file offsets 0x01ff6b to 0x0204a8, 1341 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: ..\sound\specache.c */
#include "u7port.h"
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "init.h"
#include "specard.h"
#include "oops.h"
#include "cflxbuf.h"

/* The start of a Creative Voice File: its signature and header, then its first block's. */
struct VocHeader {
	char id[8];             /* "Creative" */
	char unusedHeader[22];
	int8_t timeConstant;
	int8_t packing;
};

char *SpecacheErrorFormat = "%s line#%d";

#define SPECACHE_ERROR(line)    FatalError(SpecacheErrorFormat, __FILE__, line)

SpeechCache::SpeechCache()
{
	started = primed = 0;
	buffer = 0;
	size = 0;
	rate = 0;
	streaming = 1;
}

void SpeechCache::stop()
{
	primed = 0;
}

uint8_t SpeechCache::allocateBuffer(uint32_t n)
{
	buffer = AllocateVoodooMemory(&VoodooXmsBlock, n);
	return buffer != 0;
}

/* Forgets the buffer; the memory itself is never given back. */
void SpeechCache::freeBuffer()
{
	if (buffer != 0)
		;
	buffer = 0;
}

void SpeechCache::releaseBuffer()
{
	freeBuffer();
	size = 0;
}

/* Fill the buffer from the start of the file and queue its first block. */
void SpeechCache::prime()
{
	rate = 0;
	length = read((void *)buffer, INT32_C(32));
	pos = 0;
	primed = 0;
	if (length != 0) {
		if (checkCreativeHeader(buffer))
			length = read((void *)buffer, size);
		else
			length += read((void *)(buffer + 32), size - 32);
		pos = block * 2;
		SpeechCard.queueBlock(buffer + block * INT32_C(2), block);
		primed = 1;
	}
}

void SpeechCache::play(char *name, uint16_t blocks)
{
	if (streaming == 0)
		playWhole(name);
	else {
		if (started && blocks > 0)
			blocks = 0;
		if (!started) {
			block = DmaHalfSize;
			size = block * blocks;
			if (!allocateBuffer(size))
				ReportOutOfVoodooMemory();
			if (name != 0) {
				if (!open(name))
					ReportFileNotFound(name);
				prime();
			}
			started = 1;
		} else {
			if (isOpen())
				close();
			if (!open(name))
				ReportFileNotFound(name);
			prime();
		}
	}
}

/* Open name and grow the buffer to hold the whole file. */
void SpeechCache::playWhole(char *name)
{
	if (name == 0)
		return;
	if (isOpen())
		close();
	if (!open(name)) {
		SPECACHE_ERROR(227);
		ReportFileNotFound(name);
	}
	block = DmaHalfSize;
	length = fileSize();
	if (length > size) {
		freeBuffer();
		if (!allocateBuffer(length))
			ReportOutOfVoodooMemory();
		size = length;
	}
	prime();
}

SpeechCache::~SpeechCache()
{
	freeBuffer();
	SpeechCard.queueBlock(INT32_C(0), 0);
}

/* Whether the buffer holds a Creative Voice File; if so the card takes its time constant. */
int8_t SpeechCache::checkCreativeHeader(int32_t)
{
	int8_t ok = 0;
	VocHeader hdr;

	CopyLinearToFar(&hdr, buffer, sizeof(hdr));
	if (_fstrnicmp(hdr.id, "CREATIVE", 8) == 0) {
		SpeechCard.setTimeConstant(hdr.timeConstant);
		ok = 1;
	}
	return ok;
}

/* Queue the next block, refilling the buffer once it has all been played. */
void SpeechCache::queueNextBlock()
{
	BlockConsumed = 0;
	pos += block;
	if (pos >= size) {
		length = read((void *)buffer, size);
		pos = 0;
		SpeechCard.queueBlock(buffer + pos, block < length ? block : length);
	} else if (length < size && pos + block >= length)
		SpeechCard.queueBlock(buffer + pos, length % block);
	else
		SpeechCard.queueBlock(buffer + pos, block);
}

void SpeechCache::fillDoubleBuffer(void *to, uint16_t *firstSize, uint16_t *secondSize)
{
	CopyLinearToFar(to, buffer, (int32_t) block * 2);
	*firstSize = block;
	*secondSize = block;
}

/* Copy up to n bytes from the read position to far memory. */
uint16_t SpeechCache::readBytes(void *to, uint16_t n)
{
	uint32_t left = length - pos;

	if (n > left)
		n = left;
	CopyLinearToFar(to, buffer + pos, n);
	pos += n;
	return n;
}
