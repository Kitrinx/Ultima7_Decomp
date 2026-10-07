/* Serpent Isle SI.EXE, resident segment 141 (file offsets 0x03ec33 to 0x03ec74, 65 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "u7port.h"
#include "memhook.h"

void IgnoreVoodooAllocation(int32_t address, int32_t size)
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

void ResetMemhookGlobals(void)
{
	VoodooAllocHook = IgnoreVoodooAllocation;
}
