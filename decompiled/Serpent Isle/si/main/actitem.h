#ifndef ACTITEM_H
#define ACTITEM_H

struct far TypeFrame;
struct objref;
struct Loc;

objref far GetBackpackItem(objref ref);
objref far GetWeaponHandItem(objref ref);
objref far GetOffHandItem(objref ref);
unsigned char far PlaceNpcNearAvatar(objref *ref, Loc destinationX, Loc destinationY, unsigned char keepPath);
void far PostScheduleScript(objref *ref, char *code);
void far PostScriptToItem(objref *ref, char *code);
unsigned char far ContinueScheduleWalk(objref *ref);
unsigned far MakeTypeFrame(unsigned type, unsigned frame);
void far WalkToItemSchedule(objref *ref);
void far PickUpItemSchedule(objref *ref);
void far PutDownItemSchedule(objref *ref);
void far CarryItemSchedule(objref *ref);
unsigned char far StartWalkToItem(objref *ref, TypeFrame far &wanted, TypeFrame far &nearType, unsigned char retry);

/* Callers pass its one instance; findUseSpot never reads it. */
struct Operate {
	unsigned char far findUseSpot(objref *npc, objref target, int *spotX, int *spotY, int *spotZ, unsigned char *dir,
		int topType);
};

extern Operate UseSpotFinder;

/* The schedule handlers, indexed by work type */
typedef void (far *WorkHandler)(objref *);
extern WorkHandler WorkTypeHandlers[53];

#endif
