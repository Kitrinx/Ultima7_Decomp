/* Serpent Isle SI.EXE, overlay segment 339 (file offsets 0x09ebb0 to 0x09ee02, 594 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: cflxcach.c */
#include "cflxbuf.h"
#include "init.h"

FlexSpeechCache::FlexSpeechCache()
{
	entryIndex = 0;
	remaining = 0;
}

FlexSpeechCache::FlexSpeechCache(char)
{
	entryIndex = 0;
	remaining = 0;
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
		archive.readEntry(&entryInfo, to, 0);
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

void FlexSpeechCache::playEntry(char *name, int entry, unsigned blocks)
{
	entryIndex = entry;
	if (name && !open(name))
		AssertFail(__FILE__, 114);
}

unsigned long FlexSpeechCache::fileSize()
{
	return entrySize;
}
