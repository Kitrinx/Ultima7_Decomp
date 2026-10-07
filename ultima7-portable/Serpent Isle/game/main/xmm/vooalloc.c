/* Serpent Isle SI.EXE, resident segment 176 (file offsets 0x03f800 to 0x03f8bd, 189 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "u7port.h"
#include "vooalloc.h"
#include "memhook.h"

int32_t AllocateVoodooMemory(struct VoodooBlock *pool, int32_t size)
{
	int32_t address, padding;

	address = (pool->base + 3) & INT32_C(0xfffffffc);
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
