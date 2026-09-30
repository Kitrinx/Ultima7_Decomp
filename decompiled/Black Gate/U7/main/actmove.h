#ifndef ACTMOVE_H
#define ACTMOVE_H

struct Coord;
struct objref;

char far StowHeldItems(objref *);

void far PauseSchedule(objref *ref);
void far ArrestAvatarSchedule(objref *ref);
void far WalkToSpotSchedule(objref *ref);
void far BoardBargeSchedule(objref *ref);
void far WalkToScheduleSchedule(objref *ref);
char far IsAtScheduleLocation(objref *ref, Coord *x, Coord *y);

#endif
