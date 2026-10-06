#ifndef MANAGER_H
#define MANAGER_H

#include "cacheent.h"
#include "rescache.h"

struct RecordCache {
	char entryPool[6];
	unsigned long capacity;
	CacheManager *manager;
	unsigned long allocation;
	unsigned long end;
	CacheEntry *firstGap;
	CacheEntry *first;
	CacheEntry *mru;
	int count;
	long used;
	char checking;
	unsigned long blocks() { return count; }
	void charge(long n) { used += n; }
	int percentage() { return 100UL * used / capacity; }
};

class CacheManager {
public:
	RecordCache *own;
	virtual void setSlot(int i, CacheEntry *p) = 0;
	virtual void releaseSlot(int i, CacheEntry *p, unsigned char keep) = 0;
};

/* A table of blocks, each loaded on demand by the derived class. */
class ResourceManager : public CacheManager {
public:
	int count;
	CacheEntry **slots;
	CacheEntry *cur;
	void init(RecordCache *o, int n);
	long checksum(int i);
	void setSlot(int i, CacheEntry *p);
	void releaseSlot(int i, CacheEntry *p, unsigned char keep);
	virtual long load(int i) = 0;
	virtual long locate(int i, int j) = 0;
	unsigned char loaded(int i) { return slots[i] != 0; }
	unsigned char isCurrent() { return cur == own->mru->next; }
	void need(int i) { if (!loaded(i)) load(i); }
	void use(int i)
	{
		need(i);
		cur = slots[i];
		if (cur == 0)
			return;
		if (!isCurrent())
			Cache_touchEntry(own, cur);
	}
	void fetch(int i)
	{
		if (loaded(i))
			return;
		use(i);
	}
	void find(int i)
	{
		if (loaded(i)) {
			cur = slots[i];
		} else {
			use(i);
			cur = slots[i];
		}
	}
	void select(int i)
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
	CacheEntry *entry(int i)
	{
		use(i);
		return slots[i];
	}
	void evict(int i)
	{
		use(i);
		Cache_releaseEntry(own, slots[i]);
	}
	long get(int i)
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
