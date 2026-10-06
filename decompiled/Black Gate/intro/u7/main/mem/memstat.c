/* Black Gate U7.EXE, resident segment 139 (file offsets 0x03eeea to 0x03f026, 316 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "dosio.h"
#include "memmgr.h"
#include "memapi.h"

/* The total size of the free blocks. */
long far SumFreeFarBlocks(void)
{
	struct FarBlock far *entry;
	long address;
	long total;

	total = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = LinearToPointer(address);
		if ((entry->size & ALLOCATED) == 0)
			total += entry->size & SIZE_MASK;
	}
	return total;
}

/* The size of the largest free block. */
long far FindLargestFarBlock(void)
{
	struct FarBlock far *entry;
	long address;
	long largest;

	largest = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = LinearToPointer(address);
		if ((entry->size & ALLOCATED) == 0 && (entry->size & SIZE_MASK) > largest)
			largest = entry->size & SIZE_MASK;
	}
	return largest;
}
