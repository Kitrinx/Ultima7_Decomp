/* Black Gate U7.EXE, resident segment 14 (file offsets 0x011f87 to 0x01205a, 211 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "cflxbuf.h"
#include "rescache.h"

extern RecordCache ShapeCache;

uint8_t BorrowedSpeechCache::allocateBuffer(uint32_t size)
{
	buffer = ShrinkCache(&ShapeCache, size);
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

BorrowedSpeechCache::BorrowedSpeechCache()
{
	bufferSize = 0;
	streaming = 0;
}
