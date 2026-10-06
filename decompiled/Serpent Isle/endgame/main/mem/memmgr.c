/* Serpent Isle ENDGAME.EXE, resident segment 42 (file offsets 0x00df9e to 0x00f0d3, 4405 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <alloc.h>
#include <dos.h>
#include "dosio.h"
#include "memmgr.h"

int FarHeapReady = 0;
int FarHeapSpareEntries = 128;
long FarHeapMovedSize = 0;
long FarHeapStart, FarHeapSize, FarBlockTable;  /* heap start and size, lowest table entry */
void far *FarHeapMemory;

/* Merge a free block into the next entry's when that is free and follows on; the table closes up. */
int far MergeFreeFarBlock(long address)
{
	struct FarBlock far *entry = (struct FarBlock far *) LinearToPointer(address);

	if ((entry->size & ALLOCATED) == 0 &&
		(entry[1].size & ALLOCATED) == 0 &&
		entry->address + (entry->size & SIZE_MASK) == entry[1].address) {
		entry[1].address = entry->address;
		entry[1].size += entry->size & SIZE_MASK;
		while (address > FarBlockTable) {
			address -= ENTRY_SIZE;
			entry = (struct FarBlock far *) LinearToPointer(address);
			entry[1].address = entry->address;
			entry[1].size = entry->size;
		}
		FarBlockTable += ENTRY_SIZE;
		return 1;
	} else {
		return 0;
	}
}

/* Free a block and merge it with free neighbours. Returns where its entry ended up. */
long far ReleaseFarBlock(long address)
{
	struct FarBlock far *entry = (struct FarBlock far *) LinearToPointer(address);

	entry->size &= ~ALLOCATED;
	if (address + ENTRY_SIZE < FarHeapStart + FarHeapSize) {
		if (MergeFreeFarBlock(address))
			address += ENTRY_SIZE;
	}
	if (address > FarBlockTable)
		MergeFreeFarBlock(address - ENTRY_SIZE);
	return address;
}

/* Free every block but the retained ones; with FAR_RETAINED, start over with one free block. */
void far ResetFarBlocks(unsigned char flags)
{
	long address;
	struct FarBlock far *entry;

	if (FarHeapReady == 0)
		return;
	if (flags & FAR_RETAINED) {
		FarBlockTable = FarHeapStart + FarHeapSize - ENTRY_SIZE;
		entry = (struct FarBlock far *) LinearToPointer(FarBlockTable);
		entry->address = FarHeapStart;
		entry->size = FarHeapSize - FarHeapSpareEntries * ENTRY_SIZE;
	} else {
		for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
			entry = (struct FarBlock far *) LinearToPointer(address);
			if ((entry->size & RETAINED) == 0)
				address = ReleaseFarBlock(address);
		}
	}
}

/* Take all of DOS's far memory as the heap. */
int InitializeFarHeap(void)
{
	if (FarHeapReady == 0) {
		FarHeapSize = farcoreleft();
		FarHeapMemory = 0;
		if (FarHeapSpareEntries * ENTRY_SIZE < FarHeapSize)
			FarHeapMemory = farmalloc(FarHeapSize);
		if (FarHeapMemory != 0) {
			FarHeapReady++;
			FarHeapStart = PointerToLinear(FarHeapMemory);
			/* With 624 KB of base memory, move what lies above the heap down below 0x98000. */
			if (*(int far *) MK_FP(0x40, 0x13) == 624) {
				FarHeapMovedSize = 0x9c000L - (FarHeapSize + FarHeapStart);
				FarHeapSize = 0x98000L - (FarHeapMovedSize + FarHeapStart);
				if (FarHeapSpareEntries * ENTRY_SIZE < FarHeapSize)
					MoveFarMemory(LinearToPointer(FarHeapStart + FarHeapSize),
						LinearToPointer(0x9c000L - FarHeapMovedSize),
						(int) FarHeapMovedSize);
				else {
					farfree(FarHeapMemory);
					FarHeapReady = 0;
				}
			}
			if (FarHeapReady != 0)
				ResetFarBlocks(FAR_RETAINED);
		}
	}
	return FarHeapReady;
}

/* Put back what was moved and return the heap to DOS. */
void ReleaseFarHeapMemory(void)
{
	if (FarHeapMovedSize != 0)
		MoveFarMemory(LinearToPointer(0x9c000L - FarHeapMovedSize),
			LinearToPointer(FarHeapStart + FarHeapSize),
			(int) FarHeapMovedSize);
	if (FarHeapReady != 0)
		farfree(FarHeapMemory);
	FarHeapReady = 0;
}

/* Allocate from the top of the highest free block that fits. */
void far *far AllocateFarHeapTop(long size, unsigned char flags)
{
	long address, result, available, next, attributes;
	struct FarBlock far *entry;
	struct FarBlock far *other;

	if (size <= 0)
		return 0;
	attributes = ALLOCATED;
	if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_WORD) {
		attributes |= WORD_ALIGNED;
		size += 1;
	} else if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_PARA) {
		attributes |= PARA_ALIGNED;
		size += 15;
	}
	if (flags & FAR_RETAINED)
		attributes |= RETAINED;

	result = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE;
		address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
		available = entry->size & SIZE_MASK;
		if ((entry->size & ALLOCATED) == 0 && size <= available) {
			if (size < available) {
				if (FarHeapStart + FarHeapSize - FarHeapSpareEntries * ENTRY_SIZE >= FarBlockTable) {
					other = (struct FarBlock far *) LinearToPointer(FarHeapStart + FarHeapSize - ENTRY_SIZE);
					if ((other->size & ALLOCATED) == 0 &&
						other->address + (other->size & SIZE_MASK) == FarBlockTable &&
						(other->size & SIZE_MASK) > ENTRY_SIZE) {
						other->size -= ENTRY_SIZE;
						FarHeapSpareEntries++;
					}
				}
				if (FarHeapStart + FarHeapSize - FarHeapSpareEntries * ENTRY_SIZE < FarBlockTable) {
					FarBlockTable -= ENTRY_SIZE;
					for (next = FarBlockTable; next < address; next += ENTRY_SIZE) {
						other = (struct FarBlock far *) LinearToPointer(next);
						other->address = other[1].address;
						other->size = other[1].size;
					}
					address -= ENTRY_SIZE;
					entry = (struct FarBlock far *) LinearToPointer(address);
					entry->size -= size;
					entry[1].address += entry->size & 0xffffffL;
					entry[1].size = size + attributes;
					result = entry[1].address;
					break;
				}
			} else {
				entry->size |= attributes;
				result = entry->address;
				break;
			}
		}
	}
	if (result) {
		if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_WORD)
			result = (result + 1) & 0xfffffffeL;
		else if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_PARA)
			result = (result + 15) & 0xfffffff0L;
	}
	return LinearToPointer(result);
}

/* Allocate from the lowest free block that fits, splitting it. Aligned requests get room to round up.
 * Returns null when nothing fits. */
void far *far AllocateFarBlock(long size, unsigned char flags)
{
	long address, result, available, next, attributes;
	struct FarBlock far *entry;
	struct FarBlock far *other;

	if (flags & FAR_FROM_TOP)
		return AllocateFarHeapTop(size, flags);
	if (size <= 0)
		return 0;
	attributes = ALLOCATED;
	if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_WORD) {
		attributes |= WORD_ALIGNED;
		size += 1;
	} else if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_PARA) {
		attributes |= PARA_ALIGNED;
		size += 15;
	}
	if (flags & FAR_RETAINED)
		attributes |= RETAINED;

	result = 0;
	for (address = FarBlockTable;
		address < FarHeapStart + FarHeapSize; address += ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
		available = entry->size & SIZE_MASK;
		if ((entry->size & ALLOCATED) == 0 && size <= available) {
			if (size < available) {
				if (FarHeapStart + FarHeapSize - FarHeapSpareEntries * ENTRY_SIZE >= FarBlockTable) {
					other = (struct FarBlock far *) LinearToPointer(FarHeapStart + FarHeapSize - ENTRY_SIZE);
					if ((other->size & ALLOCATED) == 0 &&
						other->address + (other->size & SIZE_MASK) == FarBlockTable &&
						(other->size & SIZE_MASK) > ENTRY_SIZE) {
						other->size -= ENTRY_SIZE;
						FarHeapSpareEntries++;
					}
				}
				if (FarHeapStart + FarHeapSize - FarHeapSpareEntries * ENTRY_SIZE < FarBlockTable) {
					FarBlockTable -= ENTRY_SIZE;
					for (next = FarBlockTable; next < address; next += ENTRY_SIZE) {
						other = (struct FarBlock far *) LinearToPointer(next);
						other->address = other[1].address;
						other->size = other[1].size;
					}
					address -= ENTRY_SIZE;
					entry = (struct FarBlock far *) LinearToPointer(address);
					entry->size = size + attributes;
					entry[1].address += size;
					entry[1].size -= size;
					result = entry->address;
					break;
				}
			} else {
				entry->size |= attributes;
				result = entry->address;
				break;
			}
		}
	}
	if (result) {
		if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_WORD)
			result = (result + 1) & 0xfffffffeL;
		else if ((flags & FAR_ALIGN_MASK) == FAR_ALIGN_PARA)
			result = (result + 15) & 0xfffffff0L;
	}
	return LinearToPointer(result);
}

/* Free the block holding memory. */
void FindAndReleaseFarBlock(void far *memory)
{
	long address;
	long target;
	struct FarBlock far *entry;

	target = PointerToLinear(memory);
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
		if (entry->address <= target && entry->address + (entry->size & SIZE_MASK) > target) {
			ReleaseFarBlock(address);
			return;
		}
	}
}

/* The usable size of the block holding memory, or 0. */
long far MeasureFarBlock(void far *memory)
{
	long address;
	long target;
	long size;
	struct FarBlock far *entry;

	size = 0;
	target = PointerToLinear(memory);
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
		if (entry->address <= target && entry->address + (entry->size & SIZE_MASK) > target) {
			size = entry->size & SIZE_MASK;
			if (entry->size & WORD_ALIGNED)
				size -= 1;
			else if (entry->size & PARA_ALIGNED)
				size -= 15;
			break;
		}
	}
	return size;
}

/* The total size of the free blocks. */
long far SumFreeFarBlocks(void)
{
	struct FarBlock far *entry;
	long address;
	long total;

	total = 0;
	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
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
		entry = (struct FarBlock far *) LinearToPointer(address);
		if ((entry->size & ALLOCATED) == 0 && (entry->size & SIZE_MASK) > largest)
			largest = entry->size & SIZE_MASK;
	}
	return largest;
}

/* Step to the block after info->ptr, or to the first block when it is null, and describe it. */
int WalkFarHeap(struct FarHeapInfo *info)
{
	long address;
	long size;
	struct FarBlock far *entry;

	if (FarHeapReady == 0)
		return _HEAPEMPTY;
	address = FarHeapStart + FarHeapSize - ENTRY_SIZE;
	/* ptr holds a linear address until a block is found. */
	info->ptr = (void far *) PointerToLinear(info->ptr);
	for (; address >= FarBlockTable; address -= ENTRY_SIZE) {
		entry = (struct FarBlock far *) LinearToPointer(address);
		if ((long) info->ptr >= entry->address &&
			entry->address + (entry->size & SIZE_MASK) > (long) info->ptr) {
			if (info->ptr != 0)
				address -= ENTRY_SIZE;
			if (address >= FarBlockTable) {
				entry = (struct FarBlock far *) LinearToPointer(address);
				info->alignment = 0;
				info->state = 0;
				info->blockSize = size = entry->size & SIZE_MASK;
				info->ptr = info->block = LinearToPointer(entry->address);
				if (entry->size & WORD_ALIGNED) {
					info->alignment = 1;
					size -= 1;
					info->ptr = MK_FP(FP_SEG(info->ptr) + 1, (FP_OFF(info->ptr) + 1) & 14);
				} else if (entry->size & PARA_ALIGNED) {
					info->alignment = 2;
					size -= 15;
					info->ptr = MK_FP(FP_SEG(info->ptr) + 1, 0);
				}
				info->size = size;
				if (entry->size & ALLOCATED)
					info->state++;
				if (entry->size & RETAINED)
					info->state++;
				return _HEAPOK;
			}
			return _HEAPEND;
		}
	}
	return _HEAPEND;
}

/* Sort the block table by address and look for blocks that overlap. */
int CheckFarHeap(void)
{
	int count;
	long address;
	struct FarBlock far *swap;
	struct FarBlock far *entries[128];
	int i, j;

	for (address = FarHeapStart + FarHeapSize - ENTRY_SIZE, count = 0; address >= FarBlockTable;
		address -= ENTRY_SIZE, count++)
		/* The first block lands one slot below the array. */
		entries[count - 1] = (struct FarBlock far *) LinearToPointer(address);
	count--;
	for (i = 0; i < count - 1; i++) {
		for (j = count - 1; j >= i + 1; j--) {
			if (entries[j]->address < entries[j - 1]->address) {
				swap = entries[j];
				entries[j] = entries[j - 1];
				entries[j - 1] = swap;
			}
		}
	}
	for (i = 0; i < count - 1; i++) {
		if (entries[i]->address + (entries[i]->size & SIZE_MASK) > entries[i + 1]->address)
			return _HEAPCORRUPT;
	}
	return _HEAPOK;
}
