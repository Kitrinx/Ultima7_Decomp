#ifndef BOGUS_H
#define BOGUS_H

#include "objref.h"

struct Coord;

extern int EarthquakeCount;
extern unsigned char StompCounter;

void far EmptyRoutine();
void far PickWorldCoords(int x, int y, Coord *wx, Coord *wy, int *wz, char flag);
void far PickWorldCoordsOnItem(int x, int y, Coord *wx, Coord *wy, int *wz, objref *target, char flag);
void far CheatCastSpell();
int far CountHeldItems(char allParty, objref container, int type, int quality, int frame);
void far MarkItemOkayToTake(objref item);
int far TryToPlaceItem(objref target, unsigned char combine, unsigned char mark);
unsigned far GiveItemsToParty(int *count, int type, int quality, int frame, int temporary);
void far BiosScrollWindow(int down, int lines, int x1, int y1, int x2, int y2);
void far ShakeScreenForStep(void);
void far ShakeScreenForQuake(void);
void far StartEarthquake(int count);
int far GetPartyMemberIndex(objref *member);
objref far GetOuterContainer(objref current, objref stop);
unsigned char far CanCarryWeight(objref container, objref item);
unsigned char far CanHoldBulk(objref container, objref item);
void far WearOffInvisibility(objref item);

#endif
