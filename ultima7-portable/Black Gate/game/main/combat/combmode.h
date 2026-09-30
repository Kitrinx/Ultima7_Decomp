#ifndef COMBMODE_H
#define COMBMODE_H

/* Attack modes; an NPC's quality holds its current one. */
#define ATTACK_NEAREST      0
#define ATTACK_WEAKEST      1
#define ATTACK_STRONGEST    2
#define ATTACK_BERSERK      3
#define ATTACK_PROTECT      4
#define ATTACK_DEFEND       5
#define ATTACK_FLANK        6
#define ATTACK_FLEE         7
#define ATTACK_RANDOM       8
#define ATTACK_MANUAL       9

#include "npcref.h"

struct Loc;

uint8_t IsTargeting(objref *actor, objref other);
void UseSecondSpot(objref *actor);
int8_t PathToSpot(objref *actor, Loc x, Loc y, int16_t z, int16_t distance);
int8_t TeleportInCombat(objref *actor, uint8_t fleeing);
int8_t SummonMonsters(objref *actor);
void SetAttackMode(NPCRef &actor, int8_t mode);
inline void SetAttackMode(NPCRef &&actor, int8_t mode) { SetAttackMode(actor, mode); }
void SetAttackModeByRef(objref *actor, int16_t mode);

extern const int16_t SummonTypes[10];
extern const uint8_t SummonCounts[10];
extern const uint8_t SummonOdds[10];

#endif
