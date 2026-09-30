#ifndef ACTITEM_H
#define ACTITEM_H

#include "typefram.h"

struct objref;
struct Loc;

objref GetBackpackItem(objref ref);
objref GetWeaponHandItem(objref ref);
objref GetOffHandItem(objref ref);
uint8_t PlaceNpcNearAvatar(objref *ref, Loc destinationX, Loc destinationY, uint8_t keepPath);
void PostScheduleScript(objref *ref, char *code);
void PostScriptToItem(objref *ref, char *code);
uint8_t ContinueScheduleWalk(objref *ref);
uint16_t MakeTypeFrame(uint16_t type, uint16_t frame);
void WalkToItemSchedule(objref *ref);
void PickUpItemSchedule(objref *ref);
void PutDownItemSchedule(objref *ref);
void CarryItemSchedule(objref *ref);
uint8_t StartWalkToItem(objref *ref, TypeFrame &wanted, TypeFrame &nearType, uint8_t retry);
inline uint8_t StartWalkToItem(objref *ref, TypeFrame &&wanted, TypeFrame &&nearType, uint8_t retry)
{
	return StartWalkToItem(ref, wanted, nearType, retry);
}

/* Callers pass its one instance; findUseSpot never reads it. */
struct Operate {
	uint8_t findUseSpot(objref *npc, objref target, int16_t *spotX, int16_t *spotY, int16_t *spotZ, uint8_t *dir,
		int16_t topType);
};

extern Operate UseSpotFinder;

/* The schedule handlers, indexed by work type */
typedef void ( *WorkHandler)(objref *);
extern const WorkHandler WorkTypeHandlers[53];

#endif
