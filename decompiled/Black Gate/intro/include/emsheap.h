#ifndef EMSHEAP_H
#define EMSHEAP_H

/* The EMS heap: blocks in expanded memory, reached through MapEmsPointer. */
#ifdef __cplusplus
extern "C" {
#endif
void far WriteEmsHeader(void);
int far OpenEms(void);
void far CloseEms(void);
void far * far AllocateEmsBlock(long size);
void far ReleaseEmsBlock(void far *memory);
long far MeasureEmsBlock(void far *memory);
long far SumFreeEmsBlocks(void);
long far FindLargestEmsBlock(void);
#ifdef __cplusplus
}
#endif

#endif
