/* Serpent Isle SI.EXE, resident segment 45 (file offsets 0x020838 to 0x020981, 329 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "dosio.h"
#include "oops.h"
#include "cacheent.h"

void CacheEntry::reset()
{
	id = -99;
	address = -1L;
	size = 0;
	previous = 0;
	next = 0;
	older = 0;
	newer = 0;
}

void CacheEntry::clear()
{
	reset();
	id = -1;
}

void CacheEntryPool::initialize(int count)
{
	int i;

	capacity = count;
	entries = new CacheEntry[capacity];
	if (entries == 0)
		ReportOutOfNearMemory();
	for (i = 0; i < capacity; i++)
		entries[i].reset();
	for (i = 0; i < capacity; i++)
		entries[i].next = &entries[(i + 1) % capacity];
	entries[capacity - 1].next = 0;
	free = entries;
}

unsigned char CacheEntryPool::hasFree()
{
	return free != 0;
}

CacheEntry *CacheEntryPool::take()
{
	CacheEntry *entry;

	if (!hasFree())
		ReportError(0xaa02);
	entry = free;
	free = free->next;
	return entry;
}

void CacheEntryPool::giveBack(CacheEntry **entry)
{
	(*entry)->reset();
	(*entry)->next = free;
	free = *entry;
}
