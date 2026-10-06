#ifndef ACTUTIL_H
#define ACTUTIL_H

/* Where to drop an item: a direction from the NPC and a z. */
struct DropSpot { unsigned char direction, z; };

struct objref;
struct far TypeFrame;
struct NPCRef;

extern char FoodFrames[];
extern char PlateFrames[];
extern char DeskItemFrames[];
extern char KitchenItemFrames[];

/* Items an NPC carries for its schedule: find, create and remove them by type and frame. */
unsigned char far DeleteCarriedItem(objref *container, TypeFrame far &wanted);
objref far CreateCarriedItem(TypeFrame far &wanted, objref container);
objref far FindCarriedItem(objref *container, TypeFrame far &wanted);

DropSpot far FindDropSpot(objref *npc, TypeFrame far &wanted);
NPCRef far ChooseNpcInFront(objref *npc, unsigned char includeParty);
void far WakeUpNpc(objref *npc);

#endif
