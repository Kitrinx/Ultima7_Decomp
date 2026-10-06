/* Serpent Isle MAINMENU.EXE, resident segment 46 (file offsets 0x0150ba to 0x0152f9, 575 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
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
	return archive.isopen();
}

unsigned char FlexSpeechCache::open(char *name)
{
	if (archive.open(name)) {
		if (!archive.getEntry(entryIndex, &entryInfo))
			;
		else
			entrySize = remaining = entryInfo.size;
	}
	return archive.isopen();
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
