/* Black Gate U7.EXE, resident segment 9 (file offsets 0x00fbe4 to 0x00fe71, 653 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "cflxbuf.h"

FlexSpeechCache::FlexSpeechCache()
{
	entryIndex = 0;
	remaining = 0;
	entrySize = 0;
}

FlexSpeechCache::FlexSpeechCache(char streamed)
{
	entryIndex = 0;
	remaining = 0;
	entrySize = 0;
	streaming = streamed;
}

FlexSpeechCache::~FlexSpeechCache()
{
}

long FlexSpeechCache::read(void far *to, unsigned long count)
{
	if (remaining < count)
		count = remaining;
	if (count != 0) {
		entryInfo.size = count;
		archive.readEntryToVoodoo(&entryInfo, (long) to, 0);
		entryInfo.offset += count;
		remaining -= count;
	}
	return count;
}

char FlexSpeechCache::isOpen()
{
	return archive.handle >= 0;
}

unsigned char FlexSpeechCache::open(char *name)
{
	if (archive.open(name)) {
		if (!archive.getEntry(entryIndex, &entryInfo))
			;
		else
			entrySize = remaining = entryInfo.size;
	}
	return archive.handle >= 0;
}

void FlexSpeechCache::close()
{
	archive.close();
}

void FlexSpeechCache::playEntry(char *name, int entry, unsigned blocks)
{
	entryIndex = entry;
	play(name, blocks);
}

unsigned long FlexSpeechCache::fileSize()
{
	return entrySize;
}
