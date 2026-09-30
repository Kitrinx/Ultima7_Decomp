#ifndef RESCACHE_H
#define RESCACHE_H

class CacheManager;
struct RecordCache;
struct CacheEntry;

unsigned long far ShrinkCache(RecordCache *cache, long amount);
unsigned long far Cache_allocateAddress(RecordCache *cache, unsigned long size, int id);
void far Cache_releaseEntry(RecordCache *cache, CacheEntry *entry);
void far Cache_dump(RecordCache *cache);
void far Cache_reportFormatted(RecordCache *cache, char *format, ...);
void far Cache_reportError(RecordCache *cache, int code, char *where);
void far Cache_reportSizeError(RecordCache *cache, int code, int kilobytes);
CacheEntry *far Cache_getOldestEntry(RecordCache *cache);
void far Cache_touchEntry(RecordCache *cache, CacheEntry *entry);
void far Cache_ageEntry(RecordCache *cache, CacheEntry *entry);
void far Cache_checkRing(RecordCache *cache, char *where);
void far Cache_check(RecordCache *cache, char *where);
void far Cache_initialize(RecordCache *cache, CacheManager *manager, unsigned long capacity, int entries);
unsigned long far Cache_evictOldest(RecordCache *cache, unsigned long needed, CacheEntry **gap);
void far Cache_freeEntry(RecordCache *cache, CacheEntry **entry);
CacheEntry *far Cache_makeSpace(RecordCache *cache, unsigned long needed);
CacheEntry *far Cache_compact(RecordCache *cache, unsigned long needed);
CacheEntry *far Cache_findGap(RecordCache *cache, unsigned long needed, unsigned long *available);
CacheEntry *far Cache_findReleased(RecordCache *cache);
CacheEntry *far Cache_allocate(RecordCache *cache, unsigned long size, int id);
long far GetSmallerLong(long a, long b);
void far Cache_resizeEntry(RecordCache *cache, CacheEntry *entry, unsigned long size);
char far Cache_decreaseEntry(RecordCache *cache, CacheEntry **entry, unsigned long offset, unsigned long size);
char far Cache_increaseEntry(RecordCache *cache, CacheEntry **entry, unsigned long offset, unsigned long size);
void far Cache_flush(RecordCache *cache);
void far Cache_destroy(RecordCache *cache);

#endif
