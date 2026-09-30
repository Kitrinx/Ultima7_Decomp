#ifndef ACTUTIL_H
#define ACTUTIL_H

/* Where to drop an item: a direction from the NPC and a z. */
struct DropSpot { uint8_t direction, z; };

#include "typefram.h"
#include "objref.h"

struct NPCRef;

extern const char FoodFrames[];
extern const char PlateFrames[];
extern const char DeskItemFrames[];
extern const char KitchenItemFrames[];

/* Items an NPC carries for its schedule: find, create and remove them by type and frame. */
uint8_t DeleteCarriedItem(objref *container, TypeFrame &wanted);
objref CreateCarriedItem(TypeFrame &wanted, objref container);
objref FindCarriedItem(objref *container, TypeFrame &wanted);

inline uint8_t DeleteCarriedItem(objref *container, TypeFrame &&wanted) { return DeleteCarriedItem(container, wanted); }
inline objref CreateCarriedItem(TypeFrame &&wanted, objref container) { return CreateCarriedItem(wanted, container); }
inline objref FindCarriedItem(objref *container, TypeFrame &&wanted) { return FindCarriedItem(container, wanted); }

DropSpot FindDropSpot(objref *npc, TypeFrame &wanted);
inline DropSpot FindDropSpot(objref *npc, TypeFrame &&wanted) { return FindDropSpot(npc, wanted); }
NPCRef ChooseNpcInFront(objref *npc, uint8_t includeParty);
void WakeUpNpc(objref *npc);

#endif
