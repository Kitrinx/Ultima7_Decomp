/* Black Gate U7.EXE, resident segment 177 (file offsets 0x03fb86 to 0x03fc43, 189 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "vooalloc.h"

extern void ( *VoodooAllocHook)(int32_t address, int32_t size);

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
