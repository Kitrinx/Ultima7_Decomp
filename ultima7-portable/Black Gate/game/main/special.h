#ifndef SPECIAL_H
#define SPECIAL_H

struct ItemId;
struct objref;

int8_t IsInSleepFrame(ItemId item);
void LayDown(objref npc, uint8_t dying);
uint16_t GetStrangeMoverShape(uint16_t *typeFrame);
int8_t GetEffectiveHitPoints(ItemId id);

#endif
