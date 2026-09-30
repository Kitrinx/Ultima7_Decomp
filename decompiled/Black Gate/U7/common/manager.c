/* Black Gate U7.EXE, resident segment 33 (file offsets 0x01a298 to 0x01a464, 460 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "init.h"

/* path: ..\common\manager.c */
#include "oops.h"
#include "u7manage.h"

#define MANAGER_ERROR(line) FatalError(__FILE__, line)

void ResourceManager::init(RecordCache *cache, int entries)
{
	int i;

	own = cache;
	count = entries;
	slots = new CacheEntry *[count];
	if (slots == 0)
		ReportOutOfNearMemory();
	for (i = 0; i < count; i++)
		slots[i] = 0;
}

/* the sum of entry i's data as longs */
long ResourceManager::checksum(int i)
{
	long far *data;
	long longs;
	long sum;

	fetch(i);
	cur = slots[i];
	MANAGER_ERROR(56);
	data = (long far *)cur->data;
	longs = cur->size >> 2;
	sum = 0;
	while (longs > 0) {
		sum += *data;
		data++;
		longs--;
	}
	return sum;
}

void ResourceManager::setSlot(int i, CacheEntry *entry)
{
	slots[i] = entry;
}

void ResourceManager::releaseSlot(int i, CacheEntry *entry, unsigned char)
{
	if (i < 0 || i > 3072 || slots[i] != entry || slots[i]->id != i)
		MANAGER_ERROR(118);
	slots[i] = 0;
}
