#ifndef MEMMGR_H
#define MEMMGR_H

/* The far heap keeps a table of its blocks at its top, growing down. Each entry holds a block's
 * linear address and its size, with the attribute bits above the size. */
struct FarBlock {
	int32_t address;
	int32_t size;
};

#define ENTRY_SIZE 8

#define ALLOCATED INT32_C(0x80000000)
#define RETAINED INT32_C(0x40000000)
#define PARA_ALIGNED INT32_C(0x20000000)
#define WORD_ALIGNED INT32_C(0x10000000)
#define SIZE_MASK INT32_C(0xfffff)

extern int16_t FarHeapReady;
extern int32_t FarHeapStart, FarHeapSize, FarBlockTable;

int16_t MergeFreeFarBlock(int32_t address);
int32_t ReleaseFarBlock(int32_t address);
void ResetFarBlocks(uint16_t flags);
int16_t InitializeFarHeap(void);
void ReleaseFarHeapMemory(void);
void * AllocateFarHeapTop(int32_t size, uint16_t flags);
void * AllocateFarBlock(int32_t size, uint16_t flags);
void FindAndReleaseFarBlock(void *memory);
int32_t MeasureFarBlock(void *memory);

#endif
