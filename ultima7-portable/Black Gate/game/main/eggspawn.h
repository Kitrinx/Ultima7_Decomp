#ifndef EGGSPAWN_H
#define EGGSPAWN_H

struct TypeFrame;
struct Coord;
struct EggRecord;
struct objref;

extern uint8_t CategoryAttackModes[5][4];
extern uint8_t CategoryAttackOdds[5][4];
extern int16_t SpawnGroupLeader;
uint8_t SpawnMonster(int16_t source, TypeFrame &typeFrame, Coord &x, Coord &y, int16_t z,
	uint8_t alignment, int8_t workType, int16_t *count, int16_t);
uint8_t SpawnObject(int16_t source, TypeFrame &typeFrame, Coord &x, Coord &y, int16_t z, uint8_t force);
void SpawnFromEgg(EggRecord *spawn, objref source, Coord x, Coord y);

/* The egg record's hatch chance and hatched flag (defined in C). */
extern "C" {
int16_t Egg_rollChance(EggRecord *egg);
void Egg_setHatched(objref *ref);
void Egg_clearHatched(objref *ref);
}

#endif
