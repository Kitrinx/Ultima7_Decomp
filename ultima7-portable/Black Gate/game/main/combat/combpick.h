#ifndef COMBPICK_H
#define COMBPICK_H

#include "npcref.h"

int16_t FindLeaderAttacker(objref *self);
int16_t FindNearestNpc(objref *self, int16_t x, int16_t y, uint8_t alignments, int16_t modes, int8_t targeted);

struct Coord;

void RallyProtectors(NPCRef &self);
inline void RallyProtectors(NPCRef &&self) { RallyProtectors(self); }
uint8_t FindGroupCentre(objref *self, Coord *x, Coord *y, uint8_t hostile);
int16_t ChooseRandomTarget(objref *self, int8_t allowTargeted, int8_t allowHelpless, int16_t distance);
int16_t ChooseNearestTarget(objref *self, int8_t allowTargeted, int8_t allowHelpless, int16_t distance);
int16_t ChooseWeakestTarget(objref *self, int8_t allowTargeted, int8_t allowHelpless, int16_t distance);
int16_t ChooseStrongestTarget(objref *self, int8_t allowTargeted, int8_t allowHelpless, int16_t distance);
uint8_t ChooseTarget(NPCRef &self, int16_t distance);
inline uint8_t ChooseTarget(NPCRef &&self, int16_t distance) { return ChooseTarget(self, distance); }
int16_t FindIdleProtector(objref *self);
void ReassignProtector(objref *self);
void ProtectLeader(NPCRef *self);
void ChooseNearbyTarget(objref *self);

uint8_t AreEnemies(objref *self, objref other);

#endif
