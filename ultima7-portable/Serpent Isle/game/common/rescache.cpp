/* Serpent Isle SI.EXE, resident segment 67 (file offsets 0x02c833 to 0x02d72e, 3835 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Z rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "lowlevel.h"
#include "plat.h"
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
	virtual void loaded(int16_t, CacheEntry *) = 0;
	virtual void evicted(int16_t, CacheEntry *, uint8_t) = 0;
};

struct RecordCache {
	CacheEntryPool pool;
	uint32_t capacity;
	CacheManager *manager;
	uint32_t allocation;
	uint32_t end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int16_t count;
	uint32_t used;
	uint8_t checking;
	void charge(int32_t n) { used += n; }
};

/* a size asking for the whole cache */
#define WHOLE_CACHE UINT32_C(0xf0000000)

inline int8_t IsWholeCache(uint32_t size) { return size == WHOLE_CACHE; }

uint32_t Cache_allocateAddress(RecordCache *cache, uint32_t size, int16_t id)
{
	uint32_t address = Cache_allocate(cache, size, id)->address;

	return address;
}

void Cache_releaseEntry(RecordCache *cache, CacheEntry *entry)
{
	int16_t id = entry->id;

	cache->manager->evicted(id, entry, 1);
	entry->id = RELEASED_ID;
}

static void DumpLine(int16_t file, const char *format, ...)
{
	char line[100];
	va_list args;

	va_start(args, format);
	vsnprintf(line, sizeof line, format, args);
	va_end(args);
	plat_file_write(file, line, (int32_t)strlen(line));
}

void Cache_dump(RecordCache *cache)
{
	CacheEntry *entry = cache->first;
	int16_t file = plat_file_create("cache");

	if (file >= 0) {
		DumpLine(file, "Start List:\n");
		while (entry != 0) {
			DumpLine(file, "%p #%4d @%6ld, %6ld bytes\n", entry,
				entry->id, entry->address - cache->first->address, entry->size);
			DumpLine(file, entry->next == 0 || entry->next->previous == entry
				? "  |\n" : "  *           parity error!\n");
			entry = entry->next;
		}
		DumpLine(file, "End List. [%ld]\n\n", cache->capacity);
		DumpLine(file, "Start MRU:\n");
		entry = cache->mru;
		while (entry != 0) {
			DumpLine(file, "%p  @%ld\n", entry, entry->address - cache->first->address);
			DumpLine(file, entry->newer->older != entry
				? "  *           parity error!\n" : "  |\n");
			entry = entry->newer;
			if (cache->mru == entry)
				entry = 0;
		}
		DumpLine(file, "End MRU.\n\n");
		plat_file_close(file);
	}
}

void Cache_reportFormatted(RecordCache *cache, char *format, ...)
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

void Cache_reportError(RecordCache *cache, int16_t code, char *where)
{
	char message[70];

	sprintf(message, "Cache error #%04x", code);
	if (where != 0) {
		strcat(message, " at ");
		strcat(message, where);
	}
	Cache_reportFormatted(cache, message);
}

void Cache_reportSizeError(RecordCache *cache, int16_t code, int16_t kilobytes)
{
	Cache_reportFormatted(cache, "At %04x, size=%dK", code, kilobytes);
}

CacheEntry * Cache_getOldestEntry(RecordCache *cache)
{
	return cache->mru->older->older;
}

void Cache_touchEntry(RecordCache *cache, CacheEntry *entry)
{
	entry->unlinkFromAgeList();
	entry->older = cache->mru;
	entry->newer = cache->mru->newer;
	cache->mru->newer->older = entry;
	cache->mru->newer = entry;
}

void Cache_ageEntry(RecordCache *cache, CacheEntry *entry)
{
	entry->unlinkFromAgeList();
	CacheEntry *tail = cache->mru->older->older;
	entry->older = tail;
	entry->newer = tail->newer;
	tail->newer->older = entry;
	tail->newer = entry;
}

void Cache_checkRing(RecordCache *cache, char *where)
{
	CacheEntry *entry;
	int16_t count;
	uint32_t total;

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

void Cache_check(RecordCache *cache, char *where)
{
	CacheEntry *entry;
	int16_t count;
	uint32_t total;
	int16_t frames;

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

void Cache_initialize(RecordCache *cache, CacheManager *manager, uint32_t capacity, int16_t entries)
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

uint32_t Cache_evictOldest(RecordCache *cache, uint32_t needed, CacheEntry **gap)
{
	uint32_t freed;
	CacheEntry *entry;
	CacheEntry *previous;

	if (IsWholeCache(needed))
		needed = cache->capacity - 1;
	else if (needed > cache->capacity || needed < 1)
		Cache_reportSizeError(cache, 0xa023, (int16_t)needed);
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

void Cache_freeEntry(RecordCache *cache, CacheEntry **entry)
{
	CacheEntry *gap = 0;

	Cache_ageEntry(cache, *entry);
	Cache_evictOldest(cache, 1, &gap);
}

CacheEntry * Cache_makeSpace(RecordCache *cache, uint32_t needed)
{
	CacheEntry *gap;
	uint32_t reserve;

	gap = 0;
	if (IsWholeCache(needed))
		needed = cache->capacity - 1;
	else if (needed < 0 || needed > cache->capacity)
		Cache_reportSizeError(cache, 0xa021, (int16_t)(needed >> 10));
	if (needed > UINT32_C(5120))
		reserve = needed;
	else
		reserve = UINT32_C(5120);
	while (gap == 0 && cache->capacity - cache->used < reserve)
		Cache_evictOldest(cache, needed, &gap);
	Cache_check(cache, "M");
	return gap;
}

CacheEntry * Cache_compact(RecordCache *cache, uint32_t needed)
{
	uint8_t enough = 0;
	int16_t count = 0;
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

CacheEntry * Cache_findGap(RecordCache *cache, uint32_t needed, uint32_t *available)
{
	CacheEntry *entry = cache->first;
	uint32_t end = entry->address + entry->size;
	uint32_t next;

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

uint32_t ShrinkCache(RecordCache *cache, int32_t amount)
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

CacheEntry * Cache_findReleased(RecordCache *cache)
{
	int16_t count = 500;
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

CacheEntry * Cache_allocate(RecordCache *cache, uint32_t size, int16_t id)
{
	CacheEntry *gap;
	uint32_t available;
	CacheEntry *freed;
	CacheEntry *entry;

	if (IsWholeCache(size))
		size = cache->capacity - 1;
	else if (size < 0 || size > cache->capacity)
		Cache_reportSizeError(cache, 0xa025, (int16_t)(size >> 10));
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

int32_t GetSmallerLong(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

void Cache_resizeEntry(RecordCache *cache, CacheEntry *entry, uint32_t size)
{
	int16_t id;
	CacheEntry *replacement;

	if (entry->size == size)
		return;
	if (entry->size + size >= cache->capacity)
		Cache_reportSizeError(cache, 0xa022, (int16_t)((entry->size + size) >> 10));
	id = entry->id;
	Cache_releaseEntry(cache, entry);
	Cache_touchEntry(cache, entry);
	replacement = Cache_allocate(cache, size, id);
	Cache_reportError(cache, 0xa026, 0);
	Cache_freeEntry(cache, &entry);
	Cache_check(cache, "RB");
}

int8_t Cache_decreaseEntry(RecordCache *cache, CacheEntry **entry, uint32_t offset, uint32_t size)
{
	if (size < 1)
		return 0;
	Cache_reportError(cache, 0xa026, 0);
	Cache_resizeEntry(cache, *entry, (*entry)->size - size);
	return 1;
}

int8_t Cache_increaseEntry(RecordCache *cache, CacheEntry **entry, uint32_t offset, uint32_t size)
{
	if (size < 1 || (*entry)->size < offset)
		return 0;
	Cache_resizeEntry(cache, *entry, (*entry)->size + size);
	Cache_reportError(cache, 0xa026, 0);
	Cache_check(cache, "ib");
	return 1;
}

void Cache_flush(RecordCache *cache)
{
	Cache_makeSpace(cache, WHOLE_CACHE);
}

void Cache_destroy(RecordCache *cache)
{
	Cache_flush(cache);
	cache->pool.giveBack(&cache->first->next);
	cache->pool.giveBack(&cache->first);
	FreeFarHeap((void *)cache->allocation);
}
