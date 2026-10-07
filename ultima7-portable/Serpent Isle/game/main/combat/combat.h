#ifndef COMBAT_H
#define COMBAT_H

#include "u7npc.h"

struct Coord;
struct objref;

/* Combat state: each alignment's leader, a counter and a flag bit for each NPC. */
struct Combat {
	int16_t leaders[4];
	uint8_t movePoints[NPC_COUNT];
	int8_t someoneActed;
	uint16_t marks[32];
	int16_t enemyHealth, partyHealth;
	uint8_t partyAttacked, battleMusic;

	Combat();
	int16_t leader(uint8_t alignment) { return leaders[alignment]; }
	void clear(uint8_t alignment) { leaders[alignment] = -1; }
	void unmarkNPC(uint16_t);
	void markNPC(uint16_t);
	int8_t isNPCMarked(uint16_t);
	void clearMarks();
	void addMovePoints();
	void clearMovePoints();
	int8_t isProtectee(int16_t);
	int16_t countEnemies();
	void setLeader(int16_t);
	int16_t sumGroupHealth(uint8_t);
	int16_t countGroupWork(uint8_t, uint8_t);
	void playEndMusic();
	void loseSightOf(int16_t);
	void releaseProtectors(uint8_t, int16_t);
	int16_t countHostileGuards();
	void callGuards(uint8_t);
	int8_t spawnGroup(int16_t, int16_t, int16_t, int16_t, int16_t, int8_t, int8_t, int8_t, objref, int8_t);
	void makeArrivalEffect(Coord, Coord, int16_t, int16_t, int16_t);
};

extern Combat CombatGroups;

void BeginCombat(void);
void BreakOffCombat(void);

uint8_t IsAvatarInCombat(void);
uint8_t IsAvatarAutoAttack(void);

int16_t RateWeapon(objref who, objref held, int16_t minimum);
uint8_t SelectWeapon(objref who, uint8_t randomize, int16_t mode);

void ClearPartyMissileFlags(void);

#endif
