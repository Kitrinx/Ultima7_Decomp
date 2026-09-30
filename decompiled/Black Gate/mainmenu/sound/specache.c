/* Black Gate MAINMENU.EXE, resident segment 48 (file offsets 0x014f07 to 0x01540e, 1287 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

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
	char timeConstant;
	char packing;
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

unsigned char SpeechCache::allocateBuffer(unsigned long n)
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
	length = read((void far *)buffer, 32L);
	pos = 0;
	primed = 0;
	if (length != 0) {
		if (checkCreativeHeader(buffer))
			length = read((void far *)buffer, size);
		else
			length += read((void far *)(buffer + 32), size - 32);
		pos = block * 2;
		SpeechCard.queueBlock(buffer + block * 2L, block);
		primed = 1;
	}
}

void SpeechCache::play(char *name, unsigned blocks)
{
	if (streaming == 0)
		playWhole(name);
	else {
		if (started && blocks > 0)
			blocks = 0;
		if (!started) {
			block = DmaHalfSize;
			size = blocks * block;
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
	SpeechCard.queueBlock(0L, 0);
}

/* Whether the buffer holds a Creative Voice File; if so the card takes its time constant. */
char SpeechCache::checkCreativeHeader(long)
{
	char ok = 0;
	VocHeader hdr;

	MoveLinear(PointerToLinear(&hdr), buffer, sizeof(hdr), 0x111);
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
		length = read((void far *)buffer, size);
		pos = 0;
		SpeechCard.queueBlock(buffer + pos, block < length ? block : length);
	} else if (length < size && pos + block >= length)
		SpeechCard.queueBlock(buffer + pos, length % block);
	else
		SpeechCard.queueBlock(buffer + pos, block);
}

void SpeechCache::fillDoubleBuffer(void far *to, unsigned *firstSize, unsigned *secondSize)
{
	MoveLinear(PointerToLinear(to), buffer, (unsigned long) block * 2, 0x111);
	*firstSize = block;
	*secondSize = block;
}

/* Copy up to n bytes from the read position to far memory. */
unsigned SpeechCache::readBytes(void far *to, unsigned n)
{
	unsigned long left = length - pos;

	if (n > left)
		n = left;
	MoveLinear(PointerToLinear(to), buffer + pos, n, 0x111);
	pos += n;
	return n;
}
