/* Serpent Isle MAINMENU.EXE, resident segment 55 (file offsets 0x0170d8 to 0x017119, 65 bytes).
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
