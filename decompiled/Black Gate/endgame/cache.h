#ifndef CACHE_H
#define CACHE_H

#include "colbuf.h"

/* Something the cache can hold: it knows its size and reads itself into place. */
struct CacheItem {
	virtual void read(void far *buffer) = 0;
	virtual long size() = 0;
};

struct CacheList;

/* One cached item and where its bytes sit; address is null while it is not loaded. */
struct CacheNode : DoubleLink {
	CacheItem *item;
	long size;
	void far *address;
	unsigned uses;
	CacheList *list;
	void *operator new(unsigned size);
	void reset();
	void addUse();
	void far *end();
	CacheItem *getItem();
	CacheNode();
	CacheNode(CacheItem *item);
	CacheNode(CacheItem *item, CacheList *list);
};

/* A block of cache memory and the nodes loaded into it, in address order. */
struct CacheList : DoubleLink, DoubleList {
	void far *base;
	unsigned used;
	long size;
	long free;
	CacheNode *addFirst(CacheItem *item);
	CacheNode *addLast(CacheItem *item);
	void clear();
	void addUse();
	CacheList();
	CacheList(void far *base, long size);
	virtual ~CacheList();
};

/* Items kept in far or expanded memory and reloaded on demand. The last list holds the nodes
 * that are not loaded. */
struct Cache {
	DoubleList lists;
	long free;
	long size;
	void far *memory;
	unsigned char type;
	int blocks;
	CacheNode *add(CacheItem *item, char loadNow);
	int remove(CacheNode **node);
	void makeRoom(long size, CacheList *list);
	void pack();
	void findSpace(long size, void far **address, CacheNode **after, CacheList **list);
	void load(CacheNode *node);
	void unload(CacheNode *node);
	void far *lock(CacheNode *node);
	long available();
	void far *listEnd(CacheList *list);
	void dump();
	Cache();
	Cache(long size, unsigned char type);
	~Cache();
};

#endif
