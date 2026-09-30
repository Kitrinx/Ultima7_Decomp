#ifndef ROUTE_H
#define ROUTE_H

#include "objref.h"
#include "coord.h"

struct PathStore;

/* A point on a route. */
struct Waypoint {
	Coord x, y;
	char z;
	unsigned char dropped() { return x == Coord(-1); }
};

/* Where a moving item stands and where its last step took it. */
struct MovementState {
	unsigned itemType;
	Coord x, y;
	unsigned char z;
	Coord oldX, oldY;
	unsigned char oldZ;
	char zChange, state;
	unsigned char movement;
	char direction;

	MovementState();
	MovementState(objref actor) { setItem(actor); }
	unsigned char blocked() { return state == 2; }
	void setPosition(Coord nx, Coord ny, unsigned char nz) { x = nx; y = ny; z = nz; }
	void setItem(objref actor);
	void setItem(Coord nx, Coord ny, unsigned char nz) { oldX = nx; oldY = ny; oldZ = nz; }
	char tryStep(char dir, char rejectObstacle);
};

/* A mover heading for a target cell one step at a time. */
struct Walker : MovementState {
	Coord destX, destY;
	char destZ, rejectObstacle;
	Walker();
	Walker(objref, Coord, Coord, char, char);
	char chooseStep();
	void setTarget(Coord x, Coord y, char z);
	unsigned char isAtTarget();
	int getDistance();
	char getDirection();
};

/* A waypoint list built while searching. */
struct StepList {
	Waypoint list[100];
	int count;
	StepList();
	unsigned char add(Coord, Coord, char);
	unsigned char append(Waypoint *, int, int);
	void reverse();
};

/* Waypoints grouped into legs, each leg a run of the list. */
struct LegList : StepList {
	int start[100];
	int length[100];
	int current;
	LegList();
	void begin(int);
	unsigned char add(Coord, Coord, char);
	int total();
	void copy(int, PathStore *, int *);
	void drop();
};

/* A route's waypoints, kept in far memory. */
struct PathStore {
	Waypoint far *data;
	int count;
	PathStore() { data = 0; count = 0; }
	unsigned char allocate(int);
	void set(int, Waypoint *);
	unsigned char merge(StepList *, int);
	Waypoint *getWaypoint(int index);
	~PathStore();
};

/* An NPC's walk to a destination: the planned path and how far along it is. */
struct Route {
	objref actor;
	Coord destX, destY;
	char destZ;
	PathStore path;
	int index, failures;
	objref held, tracked;
	int nearest;
	unsigned char ready, rejectObstacle;
	unsigned char hold;
	unsigned char cachedValid;
	Waypoint cached;
	int planCount, attempts, limit;
	Route() { ready = 0; index = 0; }
	Route(objref, Coord, Coord, char, char, char, int, int *, char);
	~Route();
	void toggleDoor();
	void openHeldDoor();
	void closeHeldDoor();
	void checkDoorDistance();
	objref findDoor(Coord, Coord, char, char);
	char walk(int);
	unsigned char plan(unsigned char usePath, char buildPath, char avoid, int limit, int *length, int from);
	unsigned char followWall(Walker *, StepList *, char, int *, int, char);
	unsigned char straighten(StepList *, char, int *, char);
	void save(char *);
	void load(char *);
};

#endif
