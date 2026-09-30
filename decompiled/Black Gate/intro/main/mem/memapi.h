#ifndef MEMAPI_H
#define MEMAPI_H

/* The far heap. */

/* Request flags. A retained block survives a reset unless the reset also passes FAR_RETAINED. */
#define FAR_ALIGN_MASK 3
#define FAR_ALIGN_WORD 1
#define FAR_ALIGN_PARA 2
#define FAR_USE_EMS 4       /* expanded memory instead of the far heap */
#define FAR_EITHER 8        /* the other heap may serve the request too */
#define FAR_RETAINED 0x10
#define FAR_FROM_TOP 0x20

/* Expanded memory pointers carry these bits in their segment. */
#define EMS_SEGMENT_MASK 0xc000

#ifdef __cplusplus
extern "C" {
#endif
void ResetFarHeap(unsigned flags);
int StartFarHeap(int unused);
void CloseFarHeap(int unused);
void far *AllocateFarHeap(long size, int flags);
void FreeFarHeap(void far *memory);
long GetFarBlockSize(void far *memory);
long GetFarHeapFree(int unused);
long GetFarHeapLargest(int unused);
void far *AllocateFarOrFail(unsigned long size, int flags);
long LongIdentity(long value);
long far SumFreeFarBlocks(void);
long far FindLargestFarBlock(void);
#ifdef __cplusplus
}
#endif

#endif
