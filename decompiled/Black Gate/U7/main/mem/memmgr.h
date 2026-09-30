#ifndef MEMMGR_H
#define MEMMGR_H

/* The far heap keeps a table of its blocks at its top, growing down. Each entry holds a block's
 * linear address and its size, with the attribute bits above the size. */
struct FarBlock {
	long address;
	long size;
};

#define ENTRY_SIZE 8

#define ALLOCATED 0x80000000L
#define RETAINED 0x40000000L
#define PARA_ALIGNED 0x20000000L
#define WORD_ALIGNED 0x10000000L
#define SIZE_MASK 0xfffffL

extern int FarHeapReady;
extern long FarHeapStart, FarHeapSize, FarBlockTable;

int far MergeFreeFarBlock(long address);
long far ReleaseFarBlock(long address);
void far ResetFarBlocks(unsigned flags);
int InitializeFarHeap(void);
void ReleaseFarHeapMemory(void);
void far *far AllocateFarHeapTop(long size, unsigned flags);
void far *far AllocateFarBlock(long size, unsigned flags);
void FindAndReleaseFarBlock(void far *memory);
long far MeasureFarBlock(void far *memory);

#endif
