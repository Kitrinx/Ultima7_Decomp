#ifndef ITEMBUF_H
#define ITEMBUF_H

extern unsigned ItemBufferBytes;
extern int ItemBufferCount;
extern int NpcRecordCount;
extern int ExtraNpcRecordCount;
void EmptyItemBufferStub(void);

void AllocItemBuffer(int count, int npcCount, int extraCount);
void ClearChunkItemLists(void);
void BuildItemFreeList(void);
void ResetItemBuffer(void);

#endif
