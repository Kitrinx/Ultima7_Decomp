#ifndef ITEMBUF_H
#define ITEMBUF_H

extern uint16_t ItemBufferBytes;
extern int16_t ItemBufferCount;
extern int16_t NpcRecordCount;
extern int16_t ExtraNpcRecordCount;
void EmptyItemBufferStub(void);

void AllocItemBuffer(int16_t count, int16_t npcCount, int16_t extraCount);
void ClearChunkItemLists(void);
void BuildItemFreeList(void);
void ResetItemBuffer(void);

#endif
