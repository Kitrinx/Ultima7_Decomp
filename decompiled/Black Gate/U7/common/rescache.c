/* Black Gate U7.EXE, resident segment 93 (file offsets 0x0325ed to 0x0334e8, 3835 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Z rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "init.h"
#include "cacheent.h"
#include "oops.h"
#include "memapi.h"
#include "rescache.h"

class CacheManager {
	RecordCache *owner;
public:
	virtual void loaded(int, CacheEntry *) = 0;
	virtual void evicted(int, CacheEntry *, unsigned char) = 0;
};

struct RecordCache {
	CacheEntryPool pool;
	unsigned long capacity;
	CacheManager *manager;
	unsigned long allocation;
	unsigned long end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int count;
	unsigned long used;
	unsigned char checking;
	void charge(long n) { used += n; }
};

/* a size asking for the whole cache */
#define WHOLE_CACHE 0xf0000000UL

inline char IsWholeCache(unsigned long size) { return size == WHOLE_CACHE; }

unsigned long far Cache_allocateAddress(RecordCache *cache, unsigned long size, int id)
{
	unsigned long address = Cache_allocate(cache, size, id)->address;

	return address;
}

void far Cache_releaseEntry(RecordCache *cache, CacheEntry *entry)
{
	int id = entry->id;

	cache->manager->evicted(id, entry, 1);
	entry->id = RELEASED_ID;
}

void far Cache_dump(RecordCache *cache)
{
	CacheEntry *entry = cache->first;
	FILE *file = fopen("cache", "wt");

	if (file != 0) {
		fprintf(file, "Start List:\n");
		while (entry != 0) {
			fprintf(file, "%p #%4d @%6ld, %6ld bytes\n", entry,
				entry->id, entry->address - cache->first->address, entry->size);
			fprintf(file, entry->next == 0 || entry->next->previous == entry
				? "  |\n" : "  *           parity error!\n");
			entry = entry->next;
		}
		fprintf(file, "End List. [%ld]\n\n", cache->capacity);
		fprintf(file, "Start MRU:\n");
		entry = cache->mru;
		while (entry != 0) {
			fprintf(file, "%p  @%ld\n", entry, entry->address - cache->first->address);
			fprintf(file, entry->newer->older != entry
				? "  *           parity error!\n" : "  |\n");
			entry = entry->newer;
			if (cache->mru == entry)
				entry = 0;
		}
		fprintf(file, "End MRU.\n\n");
		fclose(file);
	}
}

void far Cache_reportFormatted(RecordCache *cache, char *format, ...)
{
	va_list args;
	char message[150];

	if (format != WorkString) {
		va_start(args, format);
		vsprintf(WorkString, format, args);
		va_end(args);
	}
	sprintf(message, "%s\nspace used=%ld\nshapes=%d\n", WorkString, cache->used, cache->count);
	Cache_dump(cache);
	FatalError(message);
}

void far Cache_reportError(RecordCache *cache, int code, char *where)
{
	char message[70];

	sprintf(message, "Cache error #%04x", code);
	if (where != 0) {
		strcat(message, " at ");
		strcat(message, where);
	}
	Cache_reportFormatted(cache, message);
}

void far Cache_reportSizeError(RecordCache *cache, int code, int kilobytes)
{
	Cache_reportFormatted(cache, "At %04x, size=%dK", code, kilobytes);
}

CacheEntry *far Cache_getOldestEntry(RecordCache *cache)
{
	return cache->mru->older->older;
}

void far Cache_touchEntry(RecordCache *cache, CacheEntry *entry)
{
	entry->unlinkFromAgeList();
	entry->older = cache->mru;
	entry->newer = cache->mru->newer;
	cache->mru->newer->older = entry;
	cache->mru->newer = entry;
}

void far Cache_ageEntry(RecordCache *cache, CacheEntry *entry)
{
	entry->unlinkFromAgeList();
	CacheEntry *tail = cache->mru->older->older;
	entry->older = tail;
	entry->newer = tail->newer;
	tail->newer->older = entry;
	tail->newer = entry;
}

void far Cache_checkRing(RecordCache *cache, char *where)
{
	CacheEntry *entry;
	int count;
	unsigned long total;

	if (!cache->checking)
		return;
	entry = cache->mru->newer;
	count = 0;
	total = 0;
	if (entry == 0 || entry->newer == 0)
		Cache_reportError(cache, 0xa00e, where);
	while (entry->newer != cache->mru) {
		count++;
		total += entry->size;
		if (entry->address < cache->first->address || entry->address + entry->size > cache->end)
			Cache_reportError(cache, 0xa00a, where);
		entry = entry->newer;
	}
}

void far Cache_check(RecordCache *cache, char *where)
{
	CacheEntry *entry;
	int count;
	unsigned long total;
	int frames;

	if (!cache->checking)
		return;
	entry = cache->first;
	count = 0;
	total = 0;
	while (entry != 0) {
		count++;
		total += entry->size;
		entry = entry->next;
		if (entry != 0) {
			if (entry->previous->next != entry)
				Cache_reportError(cache, 0xa008, where);
			if (entry->id > 199 && entry->id < 1024) {
				frames = GetShapeFrameCount(entry->address, 17);
				if (frames < 1 || frames > 32)
					Cache_reportFormatted(cache, "CORRUPT %d, frames=%d [%s]", entry->id, frames, where);
			}
		}
	}
	if (cache->count + 2 != count)
		Cache_reportError(cache, 0xa001, where);
	if (cache->used != total)
		Cache_reportError(cache, 0xa00c, 0);
	entry = cache->mru->newer;
	count = 0;
	total = 0;
	if (entry == 0 || entry->newer == 0)
		Cache_reportError(cache, 0xa00e, where);
	while (entry->newer != cache->mru) {
		count++;
		total += entry->size;
		entry = entry->newer;
		if (entry == 0)
			Cache_reportError(cache, 0xa00b, where);
		else if (entry->older->newer != entry)
			Cache_reportError(cache, 0xa009, where);
	}
	if (cache->count != count)
		Cache_reportError(cache, 0xa002, where);
	if (cache->used != total)
		Cache_reportError(cache, 0xa00d, where);
	if (cache->used != total)
		Cache_reportError(cache, 0xa007, where);
}

void far Cache_initialize(RecordCache *cache, CacheManager *manager, unsigned long capacity, int entries)
{
	CacheEntry *end;
	CacheEntry *start;

	cache->pool.initialize(entries);
	cache->checking = 0;
	cache->manager = manager;
	cache->capacity = capacity;
	cache->allocation = AllocateVoodooMemory(&VoodooXmsBlock, cache->capacity);
	end = cache->pool.take();
	start = cache->pool.take();
	if (cache->allocation == 0 || start == 0 || end == 0)
		ReportOutOfNearMemory();
	cache->end = 0;
	cache->firstGap = 0;
	cache->count = 0;
	cache->used = 0;
	start->clear();
	start->address = cache->allocation;
	start->next = end;
	start->older = end;
	start->newer = end;
	end->clear();
	end->address = cache->end = cache->allocation + cache->capacity;
	end->previous = start;
	end->older = start;
	end->newer = start;
	cache->mru = start;
	cache->first = start;
	Cache_check(cache, "I");
}

unsigned long far Cache_evictOldest(RecordCache *cache, unsigned long needed, CacheEntry **gap)
{
	unsigned long freed;
	CacheEntry *entry;
	CacheEntry *previous;

	if (IsWholeCache(needed))
		needed = cache->capacity - 1;
	else if (needed > cache->capacity || needed < 1)
		Cache_reportSizeError(cache, 0xa023, (int)needed);
	entry = cache->mru->older->older;
	if (cache->checking) {
		if (entry == cache->first || entry == cache->mru || entry == cache->mru->older
			|| entry == cache->first->previous)
			Cache_reportError(cache, 0xa020, 0);
	}
	if (!entry->released())
		cache->manager->evicted(entry->id, entry, 0);
	freed = entry->size;
	entry->older->newer = entry->newer;
	entry->newer->older = entry->older;
	previous = entry->previous;
	previous->next = entry->next;
	entry->next->previous = previous;
	if (cache->firstGap == 0 || entry->address < cache->firstGap->next->address) {
		if (previous == cache->first->previous)
			Cache_reportError(cache, 0xa024, 0);
		cache->firstGap = previous;
	}
	cache->charge(-entry->size);
	cache->count--;
	cache->pool.giveBack(&entry);
	if (previous->next->address - (previous->address + previous->size) > needed)
		*gap = previous;
	Cache_check(cache, "F");
	return freed;
}

void far Cache_freeEntry(RecordCache *cache, CacheEntry **entry)
{
	CacheEntry *gap = 0;

	Cache_ageEntry(cache, *entry);
	Cache_evictOldest(cache, 1, &gap);
}

CacheEntry *far Cache_makeSpace(RecordCache *cache, unsigned long needed)
{
	CacheEntry *gap;
	unsigned long reserve;

	gap = 0;
	if (IsWholeCache(needed))
		needed = cache->capacity - 1;
	else if (needed < 0 || needed > cache->capacity)
		Cache_reportSizeError(cache, 0xa021, (int)(needed >> 10));
	if (needed > 5120UL)
		reserve = needed;
	else
		reserve = 5120UL;
	while (gap == 0 && cache->capacity - cache->used < reserve)
		Cache_evictOldest(cache, needed, &gap);
	Cache_check(cache, "M");
	return gap;
}

CacheEntry *far Cache_compact(RecordCache *cache, unsigned long needed)
{
	unsigned char enough = 0;
	int count = 0;
	CacheEntry *before;
	CacheEntry *entry = cache->first;
	CacheEntry *gap;

	while (entry->next->next != cache->first && !enough) {
		before = entry;
		entry = entry->next;
		count++;
		if (entry == cache->mru->older) {
			gap = entry->previous;
			Cache_reportFormatted(cache, "Garbage %d needs %ld, has %ld", count, needed,
				gap->next->address - (gap->address + gap->size));
		}
		MoveLinearFlat(before->address + before->size, entry->address, entry->size);
		entry->address = before->address + before->size;
		enough = entry->next->address - (entry->address + entry->size) >= needed;
	}
	cache->firstGap = 0;
	Cache_check(cache, "G");
	return entry;
}

CacheEntry *far Cache_findGap(RecordCache *cache, unsigned long needed, unsigned long *available)
{
	CacheEntry *entry = cache->first;
	unsigned long end = entry->address + entry->size;
	unsigned long next;

	for (;;) {
		next = entry->next->address;
		*available = next - end;
		if (*available >= needed)
			return entry;
		if (next >= cache->end)
			return 0;
		entry = entry->next;
		end = next + entry->size;
	}
}

unsigned long far ShrinkCache(RecordCache *cache, long amount)
{
	if (amount >= 0) {
		Cache_checkRing(cache, "Pre-total-free");
		Cache_makeSpace(cache, amount);
		Cache_checkRing(cache, "Post-total-free");
		Cache_compact(cache, cache->capacity - cache->used);
		Cache_checkRing(cache, "Post-garbage-collect");
	}
	cache->mru->older->address -= amount;
	cache->end -= amount;
	cache->capacity -= amount;
	return cache->end;
}

CacheEntry *far Cache_findReleased(RecordCache *cache)
{
	int count = 500;
	CacheEntry *entry = cache->mru->older->older;

	if (entry != 0) {
		while (count != 0 && cache->mru != entry) {
			if (entry->released()) {
				Cache_ageEntry(cache, entry);
				return entry;
			}
			entry = entry->older;
			count--;
		}
		if (count == 0)
			Cache_reportError(cache, 0xa027, 0);
	}
	return 0;
}

CacheEntry *far Cache_allocate(RecordCache *cache, unsigned long size, int id)
{
	CacheEntry *gap;
	unsigned long available;
	CacheEntry *freed;
	CacheEntry *entry;

	if (IsWholeCache(size))
		size = cache->capacity - 1;
	else if (size < 0 || size > cache->capacity)
		Cache_reportSizeError(cache, 0xa025, (int)(size >> 10));
	while (!cache->pool.hasFree()) {
		Cache_findReleased(cache);
		Cache_makeSpace(cache, cache->capacity - cache->used + 1);
	}
	freed = 0;
	gap = Cache_findGap(cache, size, &available);
	Cache_check(cache, "A0");
	if (gap == 0) {
		freed = Cache_makeSpace(cache, size);
		gap = freed == 0 ? Cache_compact(cache, size) : freed;
	}
	Cache_check(cache, "A1");
	entry = cache->pool.take();
	entry->address = gap->address + gap->size;
	entry->size = size;
	entry->id = id;
	entry->next = gap->next;
	entry->previous = gap;
	gap->next->previous = entry;
	gap->next = entry;
	Cache_checkRing(cache, "A2");
	entry->older = cache->mru;
	entry->newer = cache->mru->newer;
	cache->mru->newer->older = entry;
	cache->mru->newer = entry;
	Cache_checkRing(cache, "A3");
	cache->charge(size);
	cache->count++;
	if (!entry->released())
		cache->manager->loaded(id, entry);
	return entry;
}

long far GetSmallerLong(long a, long b)
{
	return a < b ? a : b;
}

void far Cache_resizeEntry(RecordCache *cache, CacheEntry *entry, unsigned long size)
{
	int id;
	CacheEntry *replacement;

	if (entry->size == size)
		return;
	if (entry->size + size >= cache->capacity)
		Cache_reportSizeError(cache, 0xa022, (int)((entry->size + size) >> 10));
	id = entry->id;
	Cache_releaseEntry(cache, entry);
	Cache_touchEntry(cache, entry);
	replacement = Cache_allocate(cache, size, id);
	Cache_reportError(cache, 0xa026, 0);
	Cache_freeEntry(cache, &entry);
	Cache_check(cache, "RB");
}

char far Cache_decreaseEntry(RecordCache *cache, CacheEntry **entry, unsigned long offset, unsigned long size)
{
	if (size < 1)
		return 0;
	Cache_reportError(cache, 0xa026, 0);
	Cache_resizeEntry(cache, *entry, (*entry)->size - size);
	return 1;
}

char far Cache_increaseEntry(RecordCache *cache, CacheEntry **entry, unsigned long offset, unsigned long size)
{
	if (size < 1 || (*entry)->size < offset)
		return 0;
	Cache_resizeEntry(cache, *entry, (*entry)->size + size);
	Cache_reportError(cache, 0xa026, 0);
	Cache_check(cache, "ib");
	return 1;
}

void far Cache_flush(RecordCache *cache)
{
	Cache_makeSpace(cache, WHOLE_CACHE);
}

void far Cache_destroy(RecordCache *cache)
{
	Cache_flush(cache);
	cache->pool.giveBack(&cache->first->next);
	cache->pool.giveBack(&cache->first);
	FreeFarHeap((void far *)cache->allocation);
}
