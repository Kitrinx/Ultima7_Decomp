#ifndef MEMSYS_H
#define MEMSYS_H

#include "shutdown.h"
#include "reporter.h"
#include "strbuf.h"
#include "cache.h"

/* Memory types; the manager's handler table is indexed by them. */
#define NEAR_MEMORY     1
#define FAR_MEMORY      2
#define EMS_MEMORY      3
#define CACHED_MEMORY   10
#define MEMORY_TYPES    6

/* Free memory of each type, taken at one moment. */
struct MemoryStats {
	long bytes[MEMORY_TYPES];
	MemoryStats();
	void snapshot();
	long get(unsigned char type);
};

char *ReportMemory(MemoryStats *original);

/* One kind of memory. The manager reaches each through these slots. */
struct MemoryHandler : ShutdownHook, ErrorReporter {
	virtual void far *allocate(long size, unsigned char flags) = 0;
	virtual void release(void far **block) = 0;
	virtual void copy(void far *to, void far *from, unsigned size) = 0;
	virtual void far *lock(void far *block) = 0;
	virtual long available() = 0;
	virtual int version() = 0;
	virtual int checkHeap() = 0;
	virtual unsigned char type() = 0;
	virtual ~MemoryHandler() {}
	void far *allocateChecked(long size, unsigned char flags, unsigned char fail);
	void far *allocateChecked(long size, unsigned char flags, unsigned char fail, int code);
	void far *allocateChecked(long size, unsigned char flags, unsigned char fail, char *what);
	void reportFailure(unsigned char type, long size, long free);
};

/* The near heap. */
struct NearMemory : MemoryHandler {
	static unsigned char initialized;
	NearMemory();
	void far *allocate(long size, unsigned char flags);
	void release(void far **block);
	void copy(void far *to, void far *from, unsigned size);
	void far *lock(void far *block) { return block; }
	long available();
	int version() { return 0; }
	int checkHeap();
	unsigned char type() { return NEAR_MEMORY; }
	static unsigned char isInitialized();
};

/* The far heap. */
struct FarMemory : MemoryHandler {
	static unsigned char initialized;
	FarMemory();
	~FarMemory();
	void shutdown();
	void far *allocate(long size, unsigned char flags);
	void release(void far **block);
	void copy(void far *to, void far *from, unsigned size);
	void far *lock(void far *block) { return block; }
	long available();
	int version() { return 0; }
	int checkHeap();
	unsigned char type() { return FAR_MEMORY; }
	static unsigned char isInitialized();
};

/* Expanded memory, mapped into the page frame when locked. */
struct EmsMemory : MemoryHandler {
	static unsigned char initialized;
	EmsMemory();
	~EmsMemory();
	void shutdown();
	void far *allocate(long size, unsigned char flags);
	void release(void far **block);
	void copy(void far *, void far *, unsigned) {}
	void far *lock(void far *block);
	long available();
	int version();
	int checkHeap() { return 0; }
	unsigned char type() { return EMS_MEMORY; }
	static unsigned char isInitialized();
};

/* Free memory with a name; nothing sets the name. */
struct NamedStats : MemoryStats {
	Message name;
};

/* Owns one handler per memory type. Its base holds the free memory at startup. */
struct MemoryManager : NamedStats {
	static MemoryHandler *handlers[MEMORY_TYPES];
	static unsigned char instanced;
	MemoryManager();
	~MemoryManager();
	void attach(MemoryHandler *handler, int unused = 0);
	void detach(unsigned char type);
	void far *allocate(long size, unsigned char type, unsigned char flags, unsigned char fail);
	void far *allocate(long size, unsigned char type, unsigned char flags, unsigned char fail,
		int code);
	void far *allocate(long size, unsigned char type, unsigned char flags, unsigned char fail,
		char *what);
	void release(void far **block, unsigned char type, int unused = 0);
	void copy(void far *to, void far *from, long size, unsigned char type);
	void far *lock(void far *block, unsigned char type, int unused = 0);
	long available(unsigned char type, int unused = 0);
	int version(unsigned char type);
	int checkHeap(unsigned char type);
	unsigned char isAttached(unsigned char type, int unused = 0);
	void report();
	void validate(unsigned char type);
};

extern MemoryManager Memory;

/* A block's place in the cache. */
struct CacheRef {
	Cache *cache;
	CacheNode *node;
	CacheRef() { set(0, 0); }
	~CacheRef() {}
	void setNode(CacheNode *n) { node = n; }
	void set(Cache *c, CacheNode *n) { cache = c; node = n; }
	void copy(CacheRef &from) { set(from.cache, from.node); }
	void far *lock() { return cache->lock(node); }
};

/* A block of memory of any type, freed with its owner when owned. */
struct MemHandle {
	void far *data;
	unsigned char type;
	unsigned char owned;
	CacheRef cacheRef;
	MemHandle() { set(0, FAR_MEMORY, 0); }
	MemHandle(long size, unsigned char t, unsigned char flags, unsigned char fail)
		{ allocate(size, t, flags, fail); }
	~MemHandle() { release(0); }
	void set(void far *p, unsigned char t, unsigned char own)
		{ data = p; type = t; owned = own; cacheRef.setNode(0); }
	void allocate(long size, unsigned char t, unsigned char flags, unsigned char fail)
		{ set(Memory.allocate(size, t, flags, fail), t, 1); }
	void allocate(long size, unsigned char t, unsigned char flags, unsigned char fail, char *what)
		{ set(Memory.allocate(size, t, flags, fail, what), t, 1); }
	void far *lock()
	{
		if (type == CACHED_MEMORY)
			return cacheRef.lock();
		return Memory.lock(data, type);
	}
	void far *pointer() { return lock(); }
	void setCached(CacheRef &ref, unsigned char own)
		{ cacheRef.copy(ref); type = CACHED_MEMORY; owned = own; }
	void copy(MemHandle &from)
	{
		set(from.data, from.type, 0);
		if (from.type == CACHED_MEMORY)
			setCached(from.cacheRef, 0);
	}
	void takeFrom(MemHandle &from) { from.owned = 0; owned = 1; }
	int isAllocated() { return data != 0; }
	void release(int unused)
	{
		if (owned && data && type != CACHED_MEMORY)
			Memory.release(&data, type, unused);
		data = 0;
		owned = 0;
	}
};

#endif
