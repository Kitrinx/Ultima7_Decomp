/* Black Gate U7.EXE, resident segment 139 (file offsets 0x03eeea to 0x03f026, 316 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "dosio.h"
#include "memmgr.h"
#include "memapi.h"

/* The total size of the free blocks. */
int32_t SumFreeFarBlocks(void)
{
	struct FarBlock *entry;
	int32_t address;
	int32_t total;

	total = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock *) LinearToPointer(address);
		if ((entry->size & ALLOCATED) == 0)
			total += entry->size & SIZE_MASK;
	}
	return total;
}

/* The size of the largest free block. */
int32_t FindLargestFarBlock(void)
{
	struct FarBlock *entry;
	int32_t address;
	int32_t largest;

	largest = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock *) LinearToPointer(address);
		if ((entry->size & ALLOCATED) == 0 && (entry->size & SIZE_MASK) > largest)
			largest = entry->size & SIZE_MASK;
	}
	return largest;
}
