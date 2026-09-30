/* Black Gate INTRO.EXE, resident segment 52 (file offsets 0x011d33 to 0x011f75, 578 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include <dos.h>
#include "memmgr.h"
#include "memapi.h"
#include "emsheap.h"

void ResetFarHeap(unsigned flags)
{
	if (flags & FAR_USE_EMS)
		WriteEmsHeader();
	else
		ResetFarBlocks(flags);
	if (flags & FAR_EITHER) {
		if (flags & FAR_USE_EMS)
			ResetFarBlocks(flags);
		else
			WriteEmsHeader();
	}
}

int StartFarHeap(int flags)
{
	if (flags & FAR_USE_EMS)
		OpenEms();
	else
		InitializeFarHeap();
	if (flags & FAR_EITHER) {
		if (flags & FAR_USE_EMS)
			InitializeFarHeap();
		else
			OpenEms();
	}
	return 1;
}

void CloseFarHeap(int flags)
{
	if (flags & FAR_USE_EMS)
		CloseEms();
	else
		ReleaseFarHeapMemory();
	if (flags & FAR_EITHER) {
		if (flags & FAR_USE_EMS)
			ReleaseFarHeapMemory();
		else
			CloseEms();
	}
}

/* Starts the heap on first use; falls back on the other heap when allowed. */
void far *AllocateFarHeap(long size, int flags)
{
	void far *block;

	if (FarHeapReady == 0)
		InitializeFarHeap();
	if (flags & FAR_USE_EMS)
		block = AllocateEmsBlock(size);
	else
		block = AllocateFarBlock(size, flags);
	if (block == 0 && (flags & FAR_EITHER)) {
		if (flags & FAR_USE_EMS)
			block = AllocateFarBlock(size, flags);
		else
			block = AllocateEmsBlock(size);
	}
	return block;
}

void FreeFarHeap(void far *memory)
{
	if ((FP_SEG(memory) & EMS_SEGMENT_MASK) == EMS_SEGMENT_MASK)
		ReleaseEmsBlock(memory);
	else
		FindAndReleaseFarBlock(memory);
}

long GetFarBlockSize(void far *memory)
{
	if ((FP_SEG(memory) & EMS_SEGMENT_MASK) == EMS_SEGMENT_MASK)
		return MeasureEmsBlock(memory);
	return MeasureFarBlock(memory);
}

long GetFarHeapFree(int flags)
{
	long total;

	total = 0;
	total += (flags & FAR_USE_EMS) ? SumFreeEmsBlocks() : SumFreeFarBlocks();
	if (flags & FAR_EITHER)
		total += (flags & FAR_USE_EMS) ? SumFreeFarBlocks() : SumFreeEmsBlocks();
	return total;
}

long GetFarHeapLargest(int flags)
{
	long ems;
	long conventional;

	ems = conventional = 0;
	if ((flags & FAR_USE_EMS) || (flags & FAR_EITHER))
		ems = FindLargestEmsBlock();
	if (!(flags & FAR_USE_EMS) || (flags & FAR_EITHER))
		conventional = FindLargestFarBlock();
	return ems > conventional ? ems : conventional;
}
