#ifndef VOOALLOC_H
#define VOOALLOC_H

/* The XMS pool that Voodoo blocks are carved from. */
struct VoodooBlock {
	int32_t base;
	int32_t free;
	int8_t unusedFlag;
	int32_t used;
#ifdef __cplusplus
	uint8_t valid() { return base == 0 ? 0 : 1; }
#endif
};

#ifdef __cplusplus
extern "C" {
#endif
extern struct VoodooBlock VoodooXmsBlock;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C"
#endif
int32_t AllocateVoodooMemory(struct VoodooBlock *pool, int32_t size);

#endif
