/* Black Gate U7.EXE, resident segment 9 (file offsets 0x00fbe4 to 0x00fe71, 653 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "cflxbuf.h"

FlexSpeechCache::FlexSpeechCache()
{
	entryIndex = 0;
	remaining = 0;
	entrySize = 0;
}

FlexSpeechCache::FlexSpeechCache(int8_t streamed)
{
	entryIndex = 0;
	remaining = 0;
	entrySize = 0;
	streaming = streamed;
}

FlexSpeechCache::~FlexSpeechCache()
{
}

int32_t FlexSpeechCache::read(void *to, uint32_t count)
{
	if (remaining < count)
		count = remaining;
	if (count != 0) {
		entryInfo.size = count;
		archive.readEntryToVoodoo(&entryInfo, (int32_t)(intptr_t) to, 0);
		entryInfo.offset += count;
		remaining -= count;
	}
	return count;
}

int8_t FlexSpeechCache::isOpen()
{
	return archive.handle >= 0;
}

uint8_t FlexSpeechCache::open(char *name)
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

void FlexSpeechCache::playEntry(char *name, int16_t entry, uint16_t blocks)
{
	entryIndex = entry;
	play(name, blocks);
}

uint32_t FlexSpeechCache::fileSize()
{
	return entrySize;
}
