#ifndef VOOALLOC_H
#define VOOALLOC_H

#include "xmminit.h"
#include "freexmm.h"

/* The XMS pool that Voodoo blocks are carved from. */
struct VoodooBlock {
	long base;
	long free;
	char unusedFlag;
	long used;
#ifdef __cplusplus
	VoodooBlock() {}
	~VoodooBlock() {}
	unsigned char open()
	{
		base = OpenExtendedMemory();
		free = GetExtendedMemorySize();
		unusedFlag = 1;
		used = 0;
		return base == 0 ? 0 : 1;
	}
	void close() { ShutdownXMM(); }
	long getFree() { return free; }
	unsigned char valid() { return base == 0 ? 0 : 1; }
#endif
};

extern struct VoodooBlock VoodooXmsBlock;

#ifdef __cplusplus
extern "C"
#endif
long far AllocateVoodooMemory(struct VoodooBlock *pool, long size);

#endif
