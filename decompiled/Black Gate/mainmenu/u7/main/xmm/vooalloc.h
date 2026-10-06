#ifndef VOOALLOC_H
#define VOOALLOC_H

/* The XMS pool that Voodoo blocks are carved from. */
struct VoodooBlock {
	long base;
	long free;
	char unusedFlag;
	long used;
#ifdef __cplusplus
	unsigned char valid() { return base == 0 ? 0 : 1; }
#endif
};

extern struct VoodooBlock VoodooXmsBlock;

#ifdef __cplusplus
extern "C"
#endif
long far AllocateVoodooMemory(struct VoodooBlock *pool, long size);

#endif
