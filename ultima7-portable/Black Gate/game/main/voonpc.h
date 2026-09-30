#ifndef VOONPC_H
#define VOONPC_H

#include "u7npc.h"

void SaveNpcSlot(int16_t slot);

extern uint8_t NpcSlotOf[NPC_COUNT];
extern int16_t NpcItemRefs[NPC_COUNT];
extern int32_t NpcStore;
extern int16_t NpcCacheFaultIndex;
extern int16_t NpcCacheFaultValue;
extern int16_t LastNpcSlot;
uint8_t CheckNpcCache(void);
void LoadNpcSlot(uint16_t npc, int16_t slot);
#ifdef __cplusplus
extern "C" {
#endif
struct NpcBuffer *GetCachedNpcBuffer(NpcBufferPool *pool, int16_t npc);
uint8_t InitNpcCache(NpcBufferPool *pool, int16_t count);
#ifdef __cplusplus
}
#endif

extern struct NpcBuffer *NpcCacheBuffers;

#endif
