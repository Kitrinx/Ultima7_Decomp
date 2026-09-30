#ifndef CACHEENT_H
#define CACHEENT_H

/* the id of an entry its owner has let go */
#define RELEASED_ID 0x7fff

/* One block of a resource cache, on its address list and its age list. */
struct CacheEntry {
	int16_t id;
	uint32_t address, size;
	CacheEntry *previous, *next, *older, *newer;

	uint8_t released() { return id == RELEASED_ID; }
	void unlinkFromAgeList()
	{
		CacheEntry *before = older, *after = newer;
		before->newer = after;
		after->older = before;
	}
	void reset();
	void clear();
};

/* A fixed array of entries with a free list through next. */
struct CacheEntryPool {
	int16_t capacity;
	CacheEntry *entries, *free;

	void initialize(int16_t n);
	uint8_t hasFree();
	CacheEntry *take();
	void giveBack(CacheEntry **entry);
};

#endif
