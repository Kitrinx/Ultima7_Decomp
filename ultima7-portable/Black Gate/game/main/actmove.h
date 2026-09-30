#ifndef ACTMOVE_H
#define ACTMOVE_H

struct Coord;
struct objref;

int8_t StowHeldItems(objref *);

void PauseSchedule(objref *ref);
void ArrestAvatarSchedule(objref *ref);
void WalkToSpotSchedule(objref *ref);
void BoardBargeSchedule(objref *ref);
void WalkToScheduleSchedule(objref *ref);
int8_t IsAtScheduleLocation(objref *ref, Coord *x, Coord *y);

#endif
