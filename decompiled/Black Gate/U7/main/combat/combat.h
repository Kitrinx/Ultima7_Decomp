#ifndef COMBAT_H
#define COMBAT_H

#include "u7npc.h"

struct Coord;
struct objref;

/* Combat state: each alignment's leader, a counter and a flag bit for each NPC. */
struct Combat {
	int leaders[4];
	unsigned char movePoints[NPC_COUNT];
	char someoneActed;
	unsigned marks[32];
	int enemyHealth, partyHealth;
	unsigned char partyAttacked, battleMusic;

	Combat();
	int leader(unsigned char alignment) { return leaders[alignment]; }
	void clear(unsigned char alignment) { leaders[alignment] = -1; }
	void unmarkNPC(unsigned);
	void markNPC(unsigned);
	char isNPCMarked(unsigned);
	void clearMarks();
	void addMovePoints();
	void clearMovePoints();
	char isProtectee(int);
	int countEnemies();
	void setLeader(int);
	int sumGroupHealth(unsigned char);
	int countGroupWork(unsigned char, unsigned char);
	void playEndMusic();
	void loseSightOf(int);
	void releaseProtectors(unsigned char, int);
	int countHostileGuards();
	void callGuards(unsigned char);
	char spawnGroup(int, int, int, int, int, char, char, char, objref, char);
	void makeArrivalEffect(Coord, Coord, int, int, int);
};

extern Combat CombatGroups;

void BeginCombat(void);
void BreakOffCombat(void);

unsigned char IsAvatarInCombat(void);
unsigned char IsAvatarAutoAttack(void);

int far RateWeapon(objref who, objref held, int minimum);
unsigned char far SelectWeapon(objref who, unsigned char randomize, int mode);

void ClearPartyMissileFlags(void);

#endif
