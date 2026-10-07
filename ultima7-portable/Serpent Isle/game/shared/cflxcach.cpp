/* Serpent Isle MAINMENU.EXE, resident segment 46 (file offsets 0x0150ba to 0x0152f9, 575 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "cflxbuf.h"

namespace Shared {

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
	return archive.isopen();
}

uint8_t FlexSpeechCache::open(char *name)
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

void FlexSpeechCache::playEntry(char *name, int16_t entry, uint16_t blocks)
{
	entryIndex = entry;
	play(name, blocks);
}

uint32_t FlexSpeechCache::fileSize()
{
	return entrySize;
}

}
