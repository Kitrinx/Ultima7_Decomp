#ifndef MEMHOOK_H
#define MEMHOOK_H

/* Called with each Voodoo allocation's address and size. */
typedef void (far *AllocHook)(long address, long size);

extern AllocHook VoodooAllocHook;

#ifdef __cplusplus
extern "C" {
#endif
void far IgnoreVoodooAllocation(long address, long size);
void far SetVoodooAllocHook(AllocHook handler);
AllocHook far SwapVoodooAllocHook(AllocHook handler);
#ifdef __cplusplus
}
#endif

#endif
