#ifndef BOGUS_H
#define BOGUS_H

#include "objref.h"

struct Coord;

extern int16_t EarthquakeCount;
extern uint8_t StompCounter;

void EmptyRoutine();
void PickWorldCoords(int16_t x, int16_t y, Coord *wx, Coord *wy, int16_t *wz, int8_t flag);
void PickWorldCoordsOnItem(int16_t x, int16_t y, Coord *wx, Coord *wy, int16_t *wz, objref *target, int8_t flag);
void CheatCastSpell();
int16_t CountHeldItems(int8_t allParty, objref container, int16_t type, int16_t quality, int16_t frame);
void MarkItemOkayToTake(objref item);
int16_t TryToPlaceItem(objref target, uint8_t combine, uint8_t mark);
uint16_t GiveItemsToParty(int16_t count, int16_t type, int16_t quality, int16_t frame, int16_t temporary);
void BiosScrollWindow(int16_t down, int16_t lines, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void ShakeScreenForStep(void);
void ShakeScreenForQuake(void);
void StartEarthquake(int16_t count);
int16_t GetPartyMemberIndex(objref *member);
objref GetOuterContainer(objref current, objref stop);
uint8_t CanCarryWeight(objref container, objref item);
uint8_t CanHoldBulk(objref container, objref item);
void WearOffInvisibility(objref item);

#endif
