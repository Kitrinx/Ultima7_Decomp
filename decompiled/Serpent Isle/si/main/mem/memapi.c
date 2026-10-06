/* Serpent Isle SI.EXE, resident segment 136 (file offsets 0x03dbc0 to 0x03dc6c, 172 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "memmgr.h"
#include "memapi.h"

void ResetFarHeap(unsigned flags)
{
	ResetFarBlocks(flags);
}

int StartFarHeap(int unused)
{
	InitializeFarHeap();
	return 1;
}

void CloseFarHeap(int unused)
{
	ReleaseFarHeapMemory();
}

/* Starts the heap on first use. */
void far *AllocateFarHeap(long size, int flags)
{
	void far *block;

	if (FarHeapReady == 0)
		InitializeFarHeap();
	block = AllocateFarBlock(size, flags);
	return block;
}

void FreeFarHeap(void far *memory)
{
	FindAndReleaseFarBlock(memory);
}

long GetFarBlockSize(void far *memory)
{
	return MeasureFarBlock(memory);
}

/* Callers pass an argument the heap ignores. */
long GetFarHeapFree(int unused)
{
	long total;

	total = SumFreeFarBlocks();
	return total;
}

long GetFarHeapLargest(int unused)
{
	long largest;

	largest = FindLargestFarBlock();
	return largest;
}
