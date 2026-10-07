/* Serpent Isle SI.EXE, overlay segment 340 (file offsets 0x09ee20 to 0x09f002, 482 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include "lowlevel.h"
#include "cflxbuf.h"
#include "rescache.h"

extern RecordCache ShapeCache;

uint8_t BorrowedSpeechCache::allocateBuffer(uint32_t size)
{
	buffer = ShrinkCache(&ShapeCache, size);
	pos = 0;
	bufferSize = 0;
	if (buffer != 0) {
		bufferSize = size;
		return 1;
	}
	return 0;
}

void BorrowedSpeechCache::freeBuffer()
{
	if (buffer != 0)
		ShrinkCache(&ShapeCache, -bufferSize);
	bufferSize = 0;
	buffer = 0;
}

/* Read the whole entry into the buffer. */
void BorrowedSpeechCache::load()
{
	allocateBuffer(remaining);
	entryInfo.size = remaining;
	archive.readEntryToVoodoo(&entryInfo, buffer, 0);
	entryInfo.offset = 0;
	skipCreativeHeader();
}

/* A Creative Voice file starts with a 32-byte header; play from after it. */
void BorrowedSpeechCache::skipCreativeHeader()
{
	char header[32];

	CopyLinearToFar(header, buffer, INT32_C(32));
	if (_fstrnicmp(header, "CREATIVE", 8) == 0)
		pos = 32;
}

int32_t BorrowedSpeechCache::read(void *to, uint32_t count)
{
	CopyLinearToFar(to, buffer + pos, count);
	pos += count;
	return count;
}

BorrowedSpeechCache::BorrowedSpeechCache()
{
	bufferSize = 0;
	buffer = 0;
}
