#ifndef ROUTE_H
#define ROUTE_H

#include "objref.h"
#include "coord.h"

struct PathStore;

/* A point on a route. */
struct Waypoint {
	Coord x, y;
	int8_t z;
	uint8_t dropped() { return x == Coord(-1); }
};

/* Where a moving item stands and where its last step took it. */
struct MovementState {
	uint16_t itemType;
	Coord x, y;
	uint8_t z;
	Coord oldX, oldY;
	uint8_t oldZ;
	int8_t zChange, state;
	uint8_t movement;
	int8_t direction;

	MovementState();
	MovementState(objref actor) { setItem(actor); }
	uint8_t blocked() { return state == 2; }
	void setPosition(Coord nx, Coord ny, uint8_t nz) { x = nx; y = ny; z = nz; }
	void setItem(objref actor);
	void setItem(Coord nx, Coord ny, uint8_t nz) { oldX = nx; oldY = ny; oldZ = nz; }
	int8_t tryStep(int8_t dir, int8_t rejectObstacle);
};

/* A mover heading for a target cell one step at a time. */
struct Walker : MovementState {
	Coord destX, destY;
	int8_t destZ, rejectObstacle;
	Walker();
	Walker(objref, Coord, Coord, int8_t, int8_t);
	int8_t chooseStep();
	void setTarget(Coord x, Coord y, int8_t z);
	uint8_t isAtTarget();
	int16_t getDistance();
	int8_t getDirection();
};

/* A waypoint list built while searching. */
struct StepList {
	Waypoint list[100];
	int16_t count;
	StepList();
	uint8_t add(Coord, Coord, int8_t);
	uint8_t append(Waypoint *, int16_t, int16_t);
	void reverse();
};

/* Waypoints grouped into legs, each leg a run of the list. */
struct LegList : StepList {
	int16_t start[100];
	int16_t length[100];
	int16_t current;
	LegList();
	void begin(int16_t);
	uint8_t add(Coord, Coord, int8_t);
	int16_t total();
	void copy(int16_t, PathStore *, int16_t *);
	void drop();
};

/* A route's waypoints, kept in far memory. */
struct PathStore {
	Waypoint *data;
	int16_t count;
	PathStore() { data = 0; count = 0; }
	uint8_t allocate(int16_t);
	void set(int16_t, Waypoint *);
	uint8_t merge(StepList *, int16_t);
	Waypoint *getWaypoint(int16_t index);
	~PathStore();
};

/* An NPC's walk to a destination: the planned path and how far along it is. */
struct Route {
	objref actor;
	Coord destX, destY;
	int8_t destZ;
	PathStore path;
	int16_t index, failures;
	objref held, tracked;
	int16_t nearest;
	uint8_t ready, rejectObstacle;
	uint8_t hold;
	uint8_t cachedValid;
	Waypoint cached;
	int16_t planCount, attempts, limit;
	Route() { ready = 0; index = 0; }
	Route(objref, Coord, Coord, int8_t, int8_t, int8_t, int16_t, int16_t *, int8_t);
	~Route();
	void toggleDoor();
	void openHeldDoor();
	void closeHeldDoor();
	void checkDoorDistance();
	objref findDoor(Coord, Coord, int8_t, int8_t);
	int8_t walk(int16_t);
	uint8_t plan(uint8_t usePath, int8_t buildPath, int8_t avoid, int16_t limit, int16_t *length, int16_t from);
	uint8_t followWall(Walker *, StepList *, int8_t, int16_t *, int16_t, int8_t);
	uint8_t straighten(StepList *, int8_t, int16_t *, int8_t);
	void save(char *);
	void load(char *);
};

#endif
