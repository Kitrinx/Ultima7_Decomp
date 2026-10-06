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

struct NPCRef;
struct objref;
struct Loc;
struct Coord;

unsigned char far IsTargeting(objref *actor, objref other);
void far UseSecondSpot(objref *actor);
char far PathToSpot(objref *actor, Loc x, Loc y, int z, int distance);
char far TeleportInCombat(objref *actor, unsigned char fleeing);
char far SummonMonsters(objref *actor);
void far SetAttackMode(NPCRef &actor, char mode);
void far SetAttackModeByRef(objref *actor, int mode);

extern int SummonTypes[8];
extern unsigned char SummonCounts[8];
extern unsigned char SummonOdds[8];

void far SetNpcOppressor(objref attacker, objref *target);
void far SetNpcSecondSpotY(objref *actor, Coord y);
#endif
