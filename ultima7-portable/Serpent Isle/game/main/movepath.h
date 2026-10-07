#ifndef MOVEPATH_H
#define MOVEPATH_H

struct NPCRef;
struct Route;
struct Waypoint;

struct Coord;
struct objref;

void StepAvatar(int8_t direction, int16_t count);
uint8_t StartAutoroute(Coord x, Coord y, int8_t z, int16_t silent, int16_t callback, int16_t argument, int16_t event,
	int8_t discard);
int8_t ContinueAutoroute();
void SetRouteFailureUsecode(int16_t callback, int16_t argument, int16_t event);
void StopAutoroute(uint8_t reason);
int8_t WalkNPCRoute(objref npc, int16_t steps);

extern char PathFileName[];
extern NPCRef RouteOwners[15];
extern Route *NPCRoutes[15];
extern Route *Autoroute;
extern Waypoint WaypointScratch;
extern int16_t DiscardedPathLength[2];
extern int16_t ArrivalUsecode;
extern int16_t ArrivalItem;
extern int16_t ArrivalEvent;
extern int16_t FailureUsecode;
extern int16_t FailureItem;
extern int16_t FailureEvent;

#endif
