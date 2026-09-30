#ifndef RESCACHE_H
#define RESCACHE_H

class CacheManager;
struct RecordCache;
struct CacheEntry;

uint32_t ShrinkCache(RecordCache *cache, int32_t amount);
uint32_t Cache_allocateAddress(RecordCache *cache, uint32_t size, int16_t id);
void Cache_releaseEntry(RecordCache *cache, CacheEntry *entry);
void Cache_dump(RecordCache *cache);
void Cache_reportFormatted(RecordCache *cache, char *format, ...);
void Cache_reportError(RecordCache *cache, int16_t code, char *where);
void Cache_reportSizeError(RecordCache *cache, int16_t code, int16_t kilobytes);
CacheEntry * Cache_getOldestEntry(RecordCache *cache);
void Cache_touchEntry(RecordCache *cache, CacheEntry *entry);
void Cache_ageEntry(RecordCache *cache, CacheEntry *entry);
void Cache_checkRing(RecordCache *cache, char *where);
void Cache_check(RecordCache *cache, char *where);
void Cache_initialize(RecordCache *cache, CacheManager *manager, uint32_t capacity, int16_t entries);
uint32_t Cache_evictOldest(RecordCache *cache, uint32_t needed, CacheEntry **gap);
void Cache_freeEntry(RecordCache *cache, CacheEntry **entry);
CacheEntry * Cache_makeSpace(RecordCache *cache, uint32_t needed);
CacheEntry * Cache_compact(RecordCache *cache, uint32_t needed);
CacheEntry * Cache_findGap(RecordCache *cache, uint32_t needed, uint32_t *available);
CacheEntry * Cache_findReleased(RecordCache *cache);
CacheEntry * Cache_allocate(RecordCache *cache, uint32_t size, int16_t id);
int32_t GetSmallerLong(int32_t a, int32_t b);
void Cache_resizeEntry(RecordCache *cache, CacheEntry *entry, uint32_t size);
int8_t Cache_decreaseEntry(RecordCache *cache, CacheEntry **entry, uint32_t offset, uint32_t size);
int8_t Cache_increaseEntry(RecordCache *cache, CacheEntry **entry, uint32_t offset, uint32_t size);
void Cache_flush(RecordCache *cache);
void Cache_destroy(RecordCache *cache);

#endif
