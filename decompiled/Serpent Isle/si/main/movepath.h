#ifndef MOVEPATH_H
#define MOVEPATH_H

struct NPCRef;
struct Route;
struct Waypoint;

struct Coord;
struct objref;

void far StepAvatar(char direction, int count);
unsigned char far StartAutoroute(Coord x, Coord y, char z, int silent, int callback, int argument, int event,
	char discard);
char far ContinueAutoroute();
void far SetRouteFailureUsecode(int callback, int argument, int event);
void far StopAutoroute(unsigned char reason);
char far WalkNPCRoute(objref npc, int steps);

extern char PathFileName[];
extern NPCRef RouteOwners[15];
extern Route *NPCRoutes[15];
extern Route *Autoroute;
extern Waypoint WaypointScratch;
extern int DiscardedPathLength[2];
extern int ArrivalUsecode;
extern int ArrivalItem;
extern int ArrivalEvent;
extern int FailureUsecode;
extern int FailureItem;
extern int FailureEvent;

#endif
