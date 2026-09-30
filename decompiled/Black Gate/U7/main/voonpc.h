#ifndef VOONPC_H
#define VOONPC_H

#include "u7npc.h"

void SaveNpcSlot(int slot);

extern unsigned char NpcSlotOf[NPC_COUNT];
extern int NpcItemRefs[NPC_COUNT];
extern long NpcStore;
extern int NpcCacheFaultIndex;
extern int NpcCacheFaultValue;
extern int LastNpcSlot;
unsigned char CheckNpcCache(void);
void LoadNpcSlot(unsigned npc, int slot);
#ifdef __cplusplus
extern "C" {
#endif
struct NpcBuffer far *GetCachedNpcBuffer(NpcBufferPool *pool, int npc);
unsigned char InitNpcCache(NpcBufferPool *pool, int count);
#ifdef __cplusplus
}
#endif

extern struct NpcBuffer far *NpcCacheBuffers;

#endif
