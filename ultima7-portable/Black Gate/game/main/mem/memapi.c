/* Black Gate U7.EXE, resident segment 137 (file offsets 0x03e162 to 0x03e20e, 172 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "memmgr.h"
#include "memapi.h"

void ResetFarHeap(uint16_t flags)
{
	ResetFarBlocks(flags);
}

int16_t StartFarHeap(int16_t unused)
{
	InitializeFarHeap();
	return 1;
}

void CloseFarHeap(int16_t unused)
{
	ReleaseFarHeapMemory();
}

/* Starts the heap on first use. */
void *AllocateFarHeap(int32_t size, int16_t flags)
{
	void *block;

	if (FarHeapReady == 0)
		InitializeFarHeap();
	block = AllocateFarBlock(size, flags);
	return block;
}

void FreeFarHeap(void *memory)
{
	FindAndReleaseFarBlock(memory);
}

int32_t GetFarBlockSize(void *memory)
{
	return MeasureFarBlock(memory);
}

/* Callers pass an argument the heap ignores. */
int32_t GetFarHeapFree(int16_t unused)
{
	int32_t total;

	total = SumFreeFarBlocks();
	return total;
}

int32_t GetFarHeapLargest(int16_t unused)
{
	int32_t largest;

	largest = FindLargestFarBlock();
	return largest;
}
