/* Serpent Isle INTRO.EXE, resident segment 2 (file offsets 0x008f7f to 0x00903f, 192 bytes).
 * Borland C++ 2.0 -mm -1 -G -P rebuilds it byte for byte as C++.
 */

#include "vooalloc.h"
#include "memhook.h"

long AllocateVoodooMemory(struct VoodooBlock *pool, long size)
{
	long address, padding;

	address = (pool->base + 3) & 0xfffffffcL;
	padding = address - pool->base;
	if (size < 1)
		return 0;
	size += 2;
	if (pool->free < size + padding)
		return 0;
	pool->used += size + padding;
	pool->free -= size + padding;
	pool->base += size + padding;
	(*VoodooAllocHook)(address, size);
	return address;
}
