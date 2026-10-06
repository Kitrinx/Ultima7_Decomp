/* Serpent Isle INTRO.EXE, resident segment 31 (file offsets 0x00dd62 to 0x00df4e, 492 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <mem.h>
#include "memsys.h"
#include "memmgr.h"

unsigned char FarMemory::initialized = 0;

void far *FarMemory::allocate(long size, unsigned char flags)
{
	return AllocateFarBlock(size, flags);
}

void FarMemory::release(void far **block)
{
	if (*block) {
		FindAndReleaseFarBlock(*block);
		*block = 0;
	}
}

void FarMemory::copy(void far *to, void far *from, unsigned size)
{
	_fmemcpy(to, from, size);
}

long FarMemory::available()
{
	return SumFreeFarBlocks();
}

int FarMemory::checkHeap()
{
	return CheckFarHeap();
}

unsigned char FarMemory::isInitialized()
{
	return initialized;
}

void FarMemory::shutdown()
{
	ReleaseFarHeapMemory();
}

FarMemory::FarMemory()
{
	if (initialized)
		error("Illegal re-initialization of Far memory\n");
	else if (!InitializeFarHeap())
		error("Can't initialize Far memory\n");
	initialized = 1;
}

FarMemory::~FarMemory()
{
	ReleaseFarHeapMemory();
}
