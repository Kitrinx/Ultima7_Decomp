#ifndef CACHEENT_H
#define CACHEENT_H

/* the id of an entry its owner has let go */
#define RELEASED_ID 0x7fff

/* One block of a resource cache, on its address list and its age list. */
struct CacheEntry {
	int id;
	unsigned long address, size;
	CacheEntry *previous, *next, *older, *newer;

	unsigned char released() { return id == RELEASED_ID; }
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
	int capacity;
	CacheEntry *entries, *free;

	void initialize(int n);
	unsigned char hasFree();
	CacheEntry *take();
	void giveBack(CacheEntry **entry);
};

#endif
