/* Serpent Isle INTRO.EXE, resident segment 74 (file offsets 0x014bf6 to 0x014c37, 65 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include "memhook.h"

void IgnoreVoodooAllocation(long address, long size)
{
}

AllocHook VoodooAllocHook = IgnoreVoodooAllocation;

void SetVoodooAllocHook(AllocHook handler)
{
	VoodooAllocHook = handler;
}

AllocHook SwapVoodooAllocHook(AllocHook handler)
{
	AllocHook old = VoodooAllocHook;

	VoodooAllocHook = handler;
	return old;
}
