#ifndef MEMAPI_H
#define MEMAPI_H

/* The far heap. */

/* Request flags. A retained block survives a reset unless the reset also passes FAR_RETAINED. */
#define FAR_ALIGN_MASK 3
#define FAR_ALIGN_WORD 1
#define FAR_ALIGN_PARA 2
#define FAR_RETAINED 0x10
#define FAR_FROM_TOP 0x20

#ifdef __cplusplus
extern "C" {
#endif
void ResetFarHeap(uint16_t flags);
int16_t StartFarHeap(int16_t unused);
void CloseFarHeap(int16_t unused);
void *AllocateFarHeap(int32_t size, int16_t flags);
void FreeFarHeap(void *memory);
int32_t GetFarBlockSize(void *memory);
int32_t GetFarHeapFree(int16_t unused);
int32_t GetFarHeapLargest(int16_t unused);
void *AllocateFarOrFail(uint32_t size, int16_t flags);
int32_t LongIdentity(int32_t value);
int32_t SumFreeFarBlocks(void);
int32_t FindLargestFarBlock(void);
#ifdef __cplusplus
}
#endif

#endif
