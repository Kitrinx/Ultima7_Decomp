/* Serpent Isle SI.EXE, resident segment 62 (file offsets 0x02aa6f to 0x02ab10, 161 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <alloc.h>
#include "vooalloc.h"
#include "memapi.h"
#include "memfree.h"

unsigned StartupNearFree = 0;
NearMemoryInfo NearMemory;

NearMemoryInfo::NearMemoryInfo()
{
	StartupNearFree = coreleft();
}

/* the startup figure less every block now in use */
unsigned NearMemoryInfo::getNearFree()
{
	struct heapinfo hi;
	unsigned left = StartupNearFree;

	hi.ptr = 0;
	while (heapwalk(&hi) == _HEAPOK)
		if (hi.in_use)
			left -= hi.size;
	return left;
}

extern "C" void GetMemoryInfo(struct MemInfo *m)
{
	m->nearFree = NearMemory.getNearFree();
	m->farFree = GetFarHeapFree(0);
	m->highFree = VoodooXmsBlock.free;
}
