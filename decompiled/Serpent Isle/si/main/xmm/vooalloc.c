/* Serpent Isle SI.EXE, resident segment 176 (file offsets 0x03f800 to 0x03f8bd, 189 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
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
