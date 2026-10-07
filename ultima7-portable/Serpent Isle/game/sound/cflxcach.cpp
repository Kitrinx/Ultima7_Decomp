/* Serpent Isle SI.EXE, overlay segment 339 (file offsets 0x09ebb0 to 0x09ee02, 594 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: cflxcach.c */
#include "u7port.h"
#include "cflxbuf.h"
#include "init.h"

FlexSpeechCache::FlexSpeechCache()
{
	entryIndex = 0;
	remaining = 0;
}

FlexSpeechCache::FlexSpeechCache(int8_t)
{
	entryIndex = 0;
	remaining = 0;
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
		archive.readEntry(&entryInfo, to, 0);
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
		if (!archive.getEntry(entryIndex, &entryInfo)) {
			archive.close();
			return 0;
		}
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
	if (name && !open(name))
		AssertFail(__FILE__, 114);
}

uint32_t FlexSpeechCache::fileSize()
{
	return entrySize;
}
