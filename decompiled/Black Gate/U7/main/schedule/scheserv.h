#ifndef SCHESERV_H
#define SCHESERV_H

struct NPCRef;
struct far TypeFrame;

struct objref;

void far RunWaiterSchedule(objref *npc);
NPCRef far FindUnservedDiner(objref *npc, TypeFrame far &serving);
char far WalkBesideItem(objref *npc, objref *target, unsigned char dir);
void far SetDownServing(objref *npc, TypeFrame far &serving);
objref far FindItemToClear(objref *npc, TypeFrame far &wanted);
void far RemoveItemAhead(objref *npc, TypeFrame far &wanted);

#endif
