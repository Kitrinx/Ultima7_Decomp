/* Serpent Isle MAINMENU.EXE, resident segment 67 (file offsets 0x019302 to 0x0193bf, 189 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
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
