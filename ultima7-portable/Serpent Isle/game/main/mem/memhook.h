#ifndef MEMHOOK_H
#define MEMHOOK_H

/* Called with each Voodoo allocation's address and size. */
typedef void ( *AllocHook)(int32_t address, int32_t size);

extern AllocHook VoodooAllocHook;

#ifdef __cplusplus
extern "C" {
#endif
void IgnoreVoodooAllocation(int32_t address, int32_t size);
void SetVoodooAllocHook(AllocHook handler);
AllocHook SwapVoodooAllocHook(AllocHook handler);
#ifdef __cplusplus
}
#endif

#endif
