/* Black Gate U7.EXE, resident segment 142 (file offsets 0x03f1d5 to 0x03f216, 65 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from compiler flags and link order.
 */

/* Called with each Voodoo allocation's address and size. */
typedef void (far *AllocHook)(long address, long size);

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
