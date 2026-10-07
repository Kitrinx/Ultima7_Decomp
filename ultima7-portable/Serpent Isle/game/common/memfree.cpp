/* Serpent Isle SI.EXE, resident segment 62 (file offsets 0x02aa6f to 0x02ab10, 161 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "vooalloc.h"
#include "memapi.h"
#include "memfree.h"
#include <new>

/* The near heap is the host's now; report a fixed figure well above what the game checks for. */
#define NEAR_FREE 32000

uint16_t StartupNearFree = 0;
NearMemoryInfo NearMemory;

NearMemoryInfo::NearMemoryInfo()
{
	StartupNearFree = NEAR_FREE;
}

uint16_t NearMemoryInfo::getNearFree()
{
	return StartupNearFree;
}

extern "C" void GetMemoryInfo(struct MemInfo *m)
{
	m->nearFree = NearMemory.getNearFree();
	m->farFree = GetFarHeapFree(0);
	m->highFree = VoodooXmsBlock.free;
}

extern "C" void ResetMemfreeGlobals(void)
{
	StartupNearFree = 0;
}

extern "C" void ConstructMemfreeGlobals(void)
{
	new (&NearMemory) NearMemoryInfo();
}
