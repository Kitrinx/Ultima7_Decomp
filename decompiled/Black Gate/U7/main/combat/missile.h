#ifndef MISSILE_H
#define MISSILE_H

#include "misstrac.h"

struct MissileFile;
struct MissileTracker;

struct ItemId;
struct CellCoord;

extern unsigned char MissileHitThisPass;
extern unsigned FirstMissileThisPass;
extern int ActiveMissiles;
extern MissileFile MissTracFile;
extern signed char PathStepX[6];
extern signed char PathStepY[6];
extern signed char PathStepZ[6];
extern unsigned char MissileFrames[6];
extern signed char MissileFrameTurns[4][4];
unsigned char far HasLineOfFireToCoords(ItemId attacker, CellCoord x, CellCoord y, int z);
unsigned char far HasLineOfFire(ItemId attacker, ItemId target);
unsigned char far UpdateMissile(int index);

extern char *MissTracFileName;
extern long MissileGuardLow;
extern MissileTracker MissileTrackers[MISSILE_COUNT];
extern long MissileGuardHigh;

#endif
