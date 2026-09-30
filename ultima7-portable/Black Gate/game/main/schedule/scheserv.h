#ifndef SCHESERV_H
#define SCHESERV_H

#include "npcref.h"
#include "typefram.h"

void RunWaiterSchedule(objref *npc);
NPCRef FindUnservedDiner(objref *npc, TypeFrame &serving);
int8_t WalkBesideItem(objref *npc, objref *target, uint8_t dir);
void SetDownServing(objref *npc, TypeFrame &serving);
objref FindItemToClear(objref *npc, TypeFrame &wanted);
void RemoveItemAhead(objref *npc, TypeFrame &wanted);

inline NPCRef FindUnservedDiner(objref *npc, TypeFrame &&serving) { return FindUnservedDiner(npc, serving); }
inline void SetDownServing(objref *npc, TypeFrame &&serving) { SetDownServing(npc, serving); }
inline objref FindItemToClear(objref *npc, TypeFrame &&wanted) { return FindItemToClear(npc, wanted); }
inline void RemoveItemAhead(objref *npc, TypeFrame &&wanted) { RemoveItemAhead(npc, wanted); }

#endif
