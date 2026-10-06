#ifndef EGGSPAWN_H
#define EGGSPAWN_H

struct far TypeFrame;
struct Coord;
struct EggRecord;
struct objref;

extern unsigned char CategoryAttackModes[5][4];
extern unsigned char CategoryAttackOdds[5][4];
extern int SpawnGroupLeader;
unsigned char far SpawnMonster(int source, TypeFrame far &typeFrame, Coord &x, Coord &y, int z,
	unsigned char alignment, char workType, int *count, int);
unsigned char far SpawnObject(int source, TypeFrame far &typeFrame, Coord &x, Coord &y, int z, unsigned char force);
void far SpawnFromEgg(EggRecord far *spawn, objref source, Coord x, Coord y);

/* The egg record's hatch chance and hatched flag (defined in C). */
extern "C" {
int far Egg_rollChance(EggRecord far *egg);
void far Egg_setHatched(objref *ref);
void far Egg_clearHatched(objref *ref);
}

#endif
