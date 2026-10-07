#ifndef MISSILE_H
#define MISSILE_H

#include "misstrac.h"

struct MissileFile;
struct MissileTracker;

struct ItemId;
struct CellCoord;

extern uint8_t MissileHitThisPass;
extern uint16_t FirstMissileThisPass;
extern int16_t ActiveMissiles;
extern MissileFile MissTracFile;
extern const int8_t PathStepX[6];
extern const int8_t PathStepY[6];
extern const int8_t PathStepZ[6];
extern const uint8_t MissileFrames[6];
extern const int8_t MissileFrameTurns[4][4];
uint8_t HasLineOfFireToCoords(ItemId attacker, CellCoord x, CellCoord y, int16_t z);
uint8_t HasLineOfFire(ItemId attacker, ItemId target);
uint8_t UpdateMissile(int16_t index);

extern char *const MissTracFileName;
extern int32_t MissileGuardLow;
extern MissileTracker MissileTrackers[MISSILE_COUNT];
extern int32_t MissileGuardHigh;

#endif
