#ifndef SPECIAL_H
#define SPECIAL_H

struct ItemId;
struct objref;

char far IsInSleepFrame(ItemId item);
void far LayDown(objref npc, unsigned char dying);
unsigned far GetStrangeMoverShape(unsigned far *typeFrame);
char far GetEffectiveHitPoints(ItemId id);

#endif
