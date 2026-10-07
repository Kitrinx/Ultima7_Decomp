#ifndef MANAGER_H
#define MANAGER_H

#include "cacheent.h"
#include "rescache.h"

struct RecordCache {
	CacheEntryPool entryPool;
	uint32_t capacity;
	CacheManager *manager;
	uint32_t allocation;
	uint32_t end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int16_t count;
	int32_t used;
	int8_t checking;
	uint32_t blocks() { return count; }
	void charge(int32_t n) { used += n; }
	int16_t percentage() { return UINT32_C(100) * used / capacity; }
};

class CacheManager {
public:
	RecordCache *own;
	virtual void setSlot(int16_t i, CacheEntry *p) = 0;
	virtual void releaseSlot(int16_t i, CacheEntry *p, uint8_t keep) = 0;
};

/* A table of blocks, each loaded on demand by the derived class. */
class ResourceManager : public CacheManager {
public:
	int16_t count;
	CacheEntry **slots;
	CacheEntry *cur;
	void init(RecordCache *o, int16_t n);
	int32_t checksum(int16_t i);
	void setSlot(int16_t i, CacheEntry *p);
	void releaseSlot(int16_t i, CacheEntry *p, uint8_t keep);
	virtual int32_t load(int16_t i) = 0;
	virtual int32_t locate(int16_t i, int16_t j) = 0;
	uint8_t loaded(int16_t i) { return slots[i] != 0; }
	uint8_t isCurrent() { return cur == own->mru->next; }
	void need(int16_t i) { if (!loaded(i)) load(i); }
	void use(int16_t i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
	}
	void fetch(int16_t i)
	{
		if (loaded(i))
			return;
		use(i);
	}
	void find(int16_t i)
	{
		if (loaded(i)) {
			cur = slots[i];
		} else {
			use(i);
			cur = slots[i];
		}
	}
	void select(int16_t i)
	{
		if (loaded(i)) {
			cur = slots[i];
			if (!isCurrent())
				Cache_touchEntry(own, cur);
		} else {
			use(i);
			cur = slots[i];
		}
	}
	CacheEntry *entry(int16_t i)
	{
		use(i);
		return slots[i];
	}
	void evict(int16_t i)
	{
		use(i);
		Cache_releaseEntry(own, slots[i]);
	}
	int32_t get(int16_t i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return 0;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
		return cur->address;
	}
};

#endif
