/* Serpent Isle SI.EXE, resident segment 84 (file offsets 0x033583 to 0x03374f, 460 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

/* path: manager.c */
#include "u7port.h"
#include "init.h"

#include "oops.h"
#include "manager.h"
#include "arena.h"

#define MANAGER_ERROR(line) FatalError(__FILE__, line)

void ResourceManager::init(RecordCache *cache, int16_t entries)
{
	int16_t i;

	own = cache;
	count = entries;
	slots = new CacheEntry *[count];
	if (slots == 0)
		ReportOutOfNearMemory();
	for (i = 0; i < count; i++)
		slots[i] = 0;
}

/* the sum of entry i's data as longs */
int32_t ResourceManager::checksum(int16_t i)
{
	int32_t *data;
	int32_t longs;
	int32_t sum;

	fetch(i);
	cur = slots[i];
	MANAGER_ERROR(56);
	data = (int32_t *)LINEAR(cur->address);
	longs = cur->size >> 2;
	sum = 0;
	while (longs > 0) {
		sum += *data;
		data++;
		longs--;
	}
	return sum;
}

void ResourceManager::setSlot(int16_t i, CacheEntry *entry)
{
	slots[i] = entry;
}

void ResourceManager::releaseSlot(int16_t i, CacheEntry *entry, uint8_t)
{
	if (i < 0 || i > 3072 || slots[i] != entry || slots[i]->id != i)
		MANAGER_ERROR(118);
	slots[i] = 0;
}
