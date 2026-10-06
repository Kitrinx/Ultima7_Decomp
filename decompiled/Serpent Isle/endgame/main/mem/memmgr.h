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

/* Request flags. A retained block survives a reset unless the reset also passes FAR_RETAINED. */
#define FAR_ALIGN_MASK 3
#define FAR_ALIGN_WORD 1
#define FAR_ALIGN_PARA 2
#define FAR_RETAINED 0x10
#define FAR_FROM_TOP 0x20

/* One step of a walk over the heap; start with ptr null. */
struct FarHeapInfo {
	void far *ptr;              /* the block's usable start */
	void far *block;            /* where it really starts */
	long size;                  /* usable size */
	long blockSize;             /* size with alignment slack */
	unsigned char alignment;    /* 0 none, 1 word, 2 paragraph */
	unsigned char state;        /* 0 free, 1 allocated, 2 retained */
};

#ifdef __cplusplus
extern "C" {
#endif

extern int FarHeapReady;
extern long FarHeapStart, FarHeapSize, FarBlockTable;

int far MergeFreeFarBlock(long address);
long far ReleaseFarBlock(long address);
void far ResetFarBlocks(unsigned char flags);
int InitializeFarHeap(void);
void ReleaseFarHeapMemory(void);
void far *far AllocateFarHeapTop(long size, unsigned char flags);
void far *far AllocateFarBlock(long size, unsigned char flags);
void FindAndReleaseFarBlock(void far *memory);
long far MeasureFarBlock(void far *memory);
long far SumFreeFarBlocks(void);
long far FindLargestFarBlock(void);
int WalkFarHeap(struct FarHeapInfo *info);
int CheckFarHeap(void);

#ifdef __cplusplus
}
#endif

#endif
