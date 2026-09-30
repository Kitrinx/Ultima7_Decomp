/* Black Gate U7.EXE, overlay segment 247 (file offsets 0x072ab0 to 0x074d8d, 8925 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdio.h>
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "collide.h"
#include "partymov.h"
#include "chkfile.h"
#include "easyfile.h"
#include "oops.h"
#include "datanode.h"
#include "legalmov.h"
#include "type.h"
#include "coord.h"
#include "memapi.h"
#include "npcpath.h"
#include "npcref.h"
#include "route.h"
#include "movepath.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))

extern objref AvatarRef;
int16_t NearestPathDistance[2];

/* Saves and restores the routes in progress. */
struct PathSaver : DataNode {
	int16_t unused;
	char *name();
	void load(char *dir);
	void save(char *dir);
};

PathSaver SavedPaths;

char *PathSaver::name()
{
	return PathFileName;
}

void PathSaver::save(char *dir)
{
	DataFile f(BuildPath(dir, PathFileName, 0), 0);
	char used[15];
	char name[14];
	int16_t i;

	for (i = 0; i < 15; i++)
		if (NPCRoutes[i])
			used[i] = 1;
		else
			used[i] = 0;
	f.write(used, INT32_C(15));
	f.write((char *) RouteOwners, INT32_C(30));
	for (i = 0; i < 15; i++)
		if (used[i]) {
			sprintf(name, "PATH%d.DAT", i);
			NPCRoutes[i]->save(BuildPath(dir, name, 0));
		}
}

void PathSaver::load(char *dir)
{
	DataFile f(BuildPath(dir, PathFileName, 0), 1);
	char used[15];
	char name[14];
	int16_t i;

	f.read(used, INT32_C(15));
	f.read((char *) RouteOwners, INT32_C(30));
	for (i = 0; i < 15; i++) {
		if (NPCRoutes[i]) {
			delete NPCRoutes[i];
			NPCRoutes[i] = 0;
		}
		if (used[i]) {
			sprintf(name, "PATH%d.DAT", i);
			NPCRoutes[i] = new Route;
			if (NPCRoutes[i] == 0)
				ReportOutOfNearMemory();
			NPCRoutes[i]->load(BuildPath(dir, name, 0));
		}
	}
}

StepList::StepList()
{
	count = 0;
}

uint8_t StepList::add(Coord x, Coord y, int8_t z)
{
	if (count < 100) {
		list[count].x = x;
		list[count].y = y;
		list[count].z = z;
		count++;
		return 1;
	}
	return 0;
}

uint8_t StepList::append(Waypoint *from, int16_t first, int16_t last)
{
	int16_t n = last - first + 1;

	if (count + n <= 100) {
		memcpy(&list[count], &from[first], n * sizeof(Waypoint));
		count += n;
		return 1;
	}
	return 0;
}

void StepList::reverse()
{
	Waypoint t;
	int16_t half, j;
	int16_t i;

	half = count >> 1;
	j = count - 1;
	for (i = 0; i < half; i++) {
		t = list[i];
		list[i] = list[j];
		list[j] = t;
		j--;
	}
}

uint8_t PathStore::allocate(int16_t n)
{
	int32_t size;

	if (data) {
		FreeFarHeap(data);
		data = 0;
	}
	count = n;
	size = count * sizeof(Waypoint);
	if (GetFarHeapFree(0) > size + 1024)
		data = (Waypoint *) AllocateFarHeap(size, 0);
	return data != 0;
}

void PathStore::set(int16_t i, Waypoint *w)
{
	Waypoint *p = data + i;

	_fmemcpy(p, w, sizeof(Waypoint));
}

PathStore::~PathStore()
{
	if (data)
		FreeFarHeap(data);
}

/* Replaces the path up to index from with the waypoints of s still in use. */
uint8_t PathStore::merge(StepList *s, int16_t from)
{
	int16_t left, total, n;
	int16_t i;

	left = total = s->count;
	n = count - from;
	for (i = 0; i < total; i++)
		if (s->list[i].dropped())
			left--;
	if (left <= 0)
		return 0;
	Waypoint *buf = (Waypoint *) AllocateFarHeap((n + left) * sizeof(Waypoint), 0);
	if (buf == 0) {
		if (data != 0)
			FreeFarHeap(data);
		data = 0;
		count = 0;
		return 0;
	}
	int16_t j = 0;
	for (i = 0; i < total; i++)
		if (!s->list[i].dropped()) {
			_fmemcpy(buf + j, &s->list[i], sizeof(Waypoint));
			j++;
		}
	_fmemcpy(buf + left, data + from, n * sizeof(Waypoint));
	if (data != 0)
		FreeFarHeap(data);
	data = buf;
	count = n + left;
	/* Borland returned the new count, so a multiple of 256 read as failure. */
	return (uint8_t)count;
}

/* Plans a route for npc to x, y, z without walking it: whether one exists, and in *distance the
 * nearest the search came. */
uint8_t CanFindPath(objref npc, Coord x, Coord y, int8_t z, int16_t *distance, int16_t limit)
{
	int16_t avatar;
	int16_t found;
	int16_t length;
	Route *r;

	if (npc == AvatarRef)
		avatar = 1;
	else
		avatar = 0;
	r = new Route(npc, x, y, z, 0, avatar, limit, &length, 0);
	if (r == 0)
		found = 0;
	else {
		found = r->ready;
		delete r;
	}
	*distance = NearestPathDistance[0] < NearestPathDistance[1] ? NearestPathDistance[0] : NearestPathDistance[1];
	return found;
}

/* Starts npc walking to x, y, z in a free route slot: 0 started or already there, 1 no free
 * slot, 2 no route or npc already walking. */
int8_t StartPath(objref npc, CellCoord x, CellCoord y, int8_t z, int16_t limit, int16_t *length, int8_t hold)
{
	int16_t slot;

	GetNpcBufferForIbo(&npc)->result = 0;
	GetNpcBufferForIbo(&npc)->typeFlagsHigh = GetNpcBufferForIbo(&npc)->typeFlagsHigh & ~8;
	if (Item_getX(npc) == x && Item_getY(npc) == y && Item_getZ(&npc) == (uint16_t)z)
		return 0;
	for (int16_t i = 0; i < 15; i++) {
		if (RouteOwners[i] == objref(npc.off))
			return 2;
	}
	for (slot = 0; slot < 15; slot++) {
		if (NPCRoutes[slot] == 0) {
			NPCRoutes[slot] = new Route(npc, x, y, z, 1, 0, limit, length, hold);
			if (NPCRoutes[slot] == 0)
				return 2;
			if (!NPCRoutes[slot]->ready) {
				delete NPCRoutes[slot];
				NPCRoutes[slot] = 0;
				return 2;
			}
			(objref &)RouteOwners[slot] = npc;
			return 0;
		}
	}
	return 1;
}

void EndPathSlot(int16_t slot)
{
	if (NPCRoutes[slot]) {
		delete NPCRoutes[slot];
		NPCRoutes[slot] = 0;
		RouteOwners[slot].off = 0;
	}
}

uint8_t HasPath(objref npc)
{
	int16_t i;

	for (i = 0; i < 15; i++)
		if (RouteOwners[i] == objref(npc.off))
			return 1;
	return 0;
}

/* Stops every route of npc. */
void StopPaths(objref npc)
{
	int16_t i;

	if (npc.valid()) {
		GetNpcBufferForIbo(&npc)->typeFlagsHigh = GetNpcBufferForIbo(&npc)->typeFlagsHigh | 8;
		for (i = 0; i < 15; i++)
			if (RouteOwners[i] == objref(npc.off))
				EndPathSlot(i);
	}
}

Route::Route(objref who, Coord x, Coord y, int8_t z, int8_t buildPath, int8_t avoid, int16_t stepLimit, int16_t *length,
	int8_t holdDoor)
{
	actor = who;
	destX = x;
	destY = y;
	destZ = z;
	planCount = 0;
	if (GetFootprintX(ITEM(actor.off)->asTypeFrame()) == GetFootprintY(ITEM(actor.off)->asTypeFrame()))
		plan(0, buildPath, avoid, stepLimit, length, 0);
	limit = *length;
	attempts = 0;
	index = 0;
	failures = 0;
	held.off = 0;
	nearest = 999;
	hold = holdDoor;
	tracked.off = 0;
	cachedValid = 0;
}

/* Plans a walk from the actor to the destination, collecting waypoints. */
uint8_t Route::plan(uint8_t usePath, int8_t buildPath, int8_t avoid, int16_t limit, int16_t *length, int16_t from)
{
	Coord toX, toY;
	int8_t toZ;
	int8_t reversed;
	int16_t prev, dir;
	int8_t detour;
	int16_t tries, savedDir, count;
	Coord fromX, fromY;
	int8_t fromZ;

	if (planCount++ > 5)
		return 0;
	reversed = 0;
	prev = 0;
	dir = 0;
	detour = 0;
	tries = 0;
	count = 0;
	StepList s;
	NearestPathDistance[0] = NearestPathDistance[1] = 999;
	ready = 0;
	RemoveTypeFromCollision(actor);
	if (IsTypeBlockedAt(destX, destY, destZ, ITEM(actor.off)->typeFrame) || BlockedByDoor ||
		!CanTypeMoveTo(destX, destY, destZ, ITEM(actor.off)->typeFrame & 0x3ff)) {
		AddTypeToCollision(actor);
		return 0;
	}
	AddTypeToCollision(actor);
	fromX = Item_getX(actor);
	fromY = Item_getY(actor);
	fromZ = Item_getZ(&actor);
	if (destZ > fromZ) {
		reversed = 1;
		toX = fromX;
		toY = fromY;
		toZ = fromZ;
		if (usePath) {
			fromX = path.getWaypoint(from)->x;
			fromY = path.getWaypoint(from)->y;
			fromZ = path.getWaypoint(from)->z;
		} else {
			fromX = destX;
			fromY = destY;
			fromZ = destZ;
		}
	} else if (usePath) {
		toX = path.getWaypoint(from)->x;
		toY = path.getWaypoint(from)->y;
		toZ = path.getWaypoint(from)->z;
	} else {
		toX = destX;
		toY = destY;
		toZ = destZ;
	}
	if (!s.add(fromX, fromY, fromZ))
		return 0;
	Walker w(actor, toX, toY, toZ, avoid);
	w.setPosition(fromX, fromY, fromZ);
	while (!w.isAtTarget()) {
		if (count > limit && limit != -1)
			return 0;
		if (detour) {
			if (tries++ > 15)
				return 0;
			if (w.tryStep(savedDir, avoid))
				continue;
			if (!s.add(w.x, w.x, w.z))    /* passes x for y, an original bug */
				return 0;
			detour = 0;
			tries = 0;
			dir = savedDir;
			if (!followWall(&w, &s, dir, &count, limit, avoid))
				return 0;
		}
		prev = dir;
		dir = w.chooseStep();
		if (w.destZ != w.z && dir == -1) {
			detour = 1;
			savedDir = prev;
			continue;
		}
		if (dir != -1) {
			count++;
			fromX = w.x;
			fromY = w.y;
			fromZ = w.z;
			if (w.zChange || w.direction != dir && reversed) {
				if (!s.add(fromX, fromY, fromZ))
					return 0;
			}
		} else if (!followWall(&w, &s, w.getDirection(), &count, limit, avoid))
			return 0;
	}
	if (!s.add(toX, toY, toZ))
		return 0;
	if (reversed)
		s.reverse();
	if (buildPath) {
		rejectObstacle = avoid;
		int16_t ok = straighten(&s, avoid, length, !usePath);
		if (usePath && !path.merge(&s, from + 1))
			return 0;
		return ok;
	}
	ready = 1;
	/* Borland returned add's 1, or the half count reverse left in AX. */
	return reversed ? (uint8_t)(s.count >> 1) : 1;
}

/* Search from both ends of the wall: walker 0 turns one way, walker 1 the other. */
uint8_t Route::followWall(Walker *w, StepList *out, int8_t heading, int16_t *count, int16_t limit, int8_t avoid)
{
	Coord x, x2, y, y2;
	int8_t z, z2;
	int16_t dir[2], turn[2];
	int16_t dist, d, dx, dy, next;
	int16_t i, j;

	StepList trail[2];
	trail[0].count = 0;
	trail[1].count = 0;
	NearestPathDistance[0] = NearestPathDistance[1] = w->z == w->destZ ? w->getDistance() : 9999;
	dir[0] = dir[1] = heading;
	turn[0] = turn[1] = 0;
	Walker end[2];
	end[0].setItem(actor);
	end[1].setItem(actor);
	end[0].setPosition(w->x, w->y, w->z);
	end[1].setPosition(w->x, w->y, w->z);
	end[0].setTarget(w->destX, w->destY, w->destZ);
	end[1].setTarget(w->destX, w->destY, w->destZ);
	out->add(w->x, w->y, w->z);
	for (;;) {
		if (*count > limit && limit != -1)
			return 0;
		for (i = 0; i < 2; i++) {
			if (i == 0) {
				for (j = 0; j < 8; j++) {
					d = (dir[i] + j - turn[i] + 8) & 7;
					if (end[i].tryStep(d, avoid))
						goto moved;
				}
			} else {
				for (j = 8; j > 0; j--) {
					d = (dir[i] + j + turn[i] + 8) & 7;
					if (end[i].tryStep(d, avoid))
						goto moved;
				}
			}
			return 0;
moved:
			(*count)++;
			if (dir[i] != d || end[i].zChange) {
				if (!trail[i].add(end[i].x, end[i].y, end[i].z))
					return 0;
				dir[i] = d;
			}
			turn[i] = (dir[i] & 1) + 1;
			x = end[i].x;
			y = end[i].y;
			z = end[i].z;
			if (end[i].isAtTarget())
				goto arrived;
			if (end[i].destZ == z) {
				dist = end[i].getDistance();
				if (NearestPathDistance[i] >= dist) {
					dx = GetDelta(end[i].destX, x);
					dy = GetDelta(end[i].destY, y);
					next = DirectionBySign[GetSign(dx) + 1][GetSign(dy) + 1];
					NearestPathDistance[i] = dist;
					x = end[i].x;
					y = end[i].y;
					z = end[i].z;
					if (end[i].tryStep(next, avoid) && end[i].zChange == 0) {
arrived:
						if (!trail[i].add(end[i].x, end[i].y, end[i].z))
							return 0;
						out->append(trail[i].list, 0, trail[i].count - 1);
						w->setPosition(x, y, z);
						return 1;
					} else {
						end[i].setPosition(x, y, z);
					}
				}
			}
			x = end[0].x;
			x2 = end[1].x;
			y = end[0].y;
			y2 = end[1].y;
			z = end[0].z;
			z2 = end[1].z;
			if (x == x2 && y == y2 && z == z2 && dir[0] != dir[1]) {
				if (i == 0) {
					for (j = 0; j < 8; j++) {
						d = (dir[i] + j - turn[i] + 8) & 7;
						if (((dir[1] + 4) & 7) == d)
							return 0;
						if (end[i].tryStep(d, avoid)) {
							end[i].setPosition(x, y, z);
							goto next_end;
						}
					}
				} else {
					for (j = 8; j > 0; j--) {
						d = (dir[i] + j + turn[i] + 8) & 7;
						if (((dir[0] + 4) & 7) == d)
							return 0;
						if (end[i].tryStep(d, avoid)) {
							end[i].setPosition(x2, y2, z2);
							goto next_end;
						}
					}
				}
				return 0;
			}
next_end:
			;
		}
	}
}

LegList::LegList()
{
	memset(start, 0, sizeof start);
	memset(length, 0, sizeof length);
	current = 0;
}

void LegList::begin(int16_t leg)
{
	start[leg] = count;
	current = leg;
}

uint8_t LegList::add(Coord x, Coord y, int8_t z)
{
	length[current]++;
	return StepList::add(x, y, z);
}

int16_t LegList::total()
{
	int16_t sum = 0;
	int16_t i;

	for (i = 0; i <= current; i++)
		sum += length[i];
	return sum;
}

/* Appends leg's waypoints to path from *pos on. */
void LegList::copy(int16_t leg, PathStore *path, int16_t *pos)
{
	int16_t end = length[leg] + start[leg];
	int16_t i;

	for (i = start[leg]; i < end; i++) {
		path->set(*pos, &list[i]);
		(*pos)++;
	}
}

void LegList::drop()
{
	count -= length[current];
	length[current] = 0;
}

/* Shortens the route by walking straight between distant waypoints, then stores it. */
uint8_t Route::straighten(StepList *list, int8_t avoid, int16_t *length, int8_t store)
{
	int16_t j;
	int16_t k;
	int8_t dir;
	Waypoint last;
	uint8_t stepped;
	uint8_t pending;
	int16_t n;
	int16_t left;
	int16_t i;

	ready = 0;
	LegList found;
	left = n = list->count;
	Walker w(actor, 0, 0, 0, avoid);
	stepped = pending = 0;
	for (i = 0; i < n - 1; i++) {
		if (list->list[i].dropped())
			continue;
		found.begin(i);
		if (stepped)
			pending = 1;
		last.x = -1;
		for (j = n - 1; j > i; j--) {
			if (list->list[j].dropped())
				continue;
			if (pending)
				stepped = 1;
			w.setPosition(list->list[i].x, list->list[i].y, list->list[i].z);
			w.setTarget(list->list[j].x, list->list[j].y, list->list[j].z);
			uint8_t shortcut = 1;
			while (!w.isAtTarget()) {
				dir = w.chooseStep();
				if (w.blocked()) {
					if ((MAX(GetMagnitude(GetDelta(list->list[i].x, w.x)),
						GetMagnitude(GetDelta(list->list[i].y, w.y))) > 3 ||
						list->list[i].z != w.z) && Coord(-1) != last.x) {
						if (!found.add(last.x, last.y, last.z))
							return 0;
					}
					stepped = 1;
				} else if (stepped) {
					if (list->list[i].x == w.x && list->list[i].y == w.y &&
						list->list[i].z == w.z) {
						shortcut = 0;
						break;
					}
					if (!found.add(w.x, w.y, w.z))
						return 0;
					stepped = 0;
				}
				last.x = w.x;
				last.y = w.y;
				last.z = w.z;
				if (dir == -1) {
					found.drop();
					shortcut = 0;
					break;
				}
			}
			if (shortcut) {
				for (k = i + 1; k < j; k++) {
					found.length[k] = 0;
					list->list[k].x = -1;
					left--;
				}
			}
		}
	}
	if (store) {
		index = 0;
		*length = 0;
		if (path.allocate(left + found.total())) {
			int16_t out = 0;
			for (i = 0; i < n; i++) {
				if (!list->list[i].dropped()) {
					if (i > 0)
						*length += MAX(GetMagnitude(GetDelta(list->list[i].x, list->list[i - 1].x)),
							GetMagnitude(GetDelta(list->list[i].y, list->list[i - 1].y)));
					path.set(out, &list->list[i]);
					out++;
					found.copy(i, &path, &out);
				}
			}
			ready = 1;
		} else
			return 0;
	}
	return 1;
}

void Route::save(char *name)
{
	DataFile f(name, 0);

	f.writeWord(planCount);
	f.writeWord(attempts);
	f.writeWord(limit);
	f.writeWord(cachedValid);
	f.write((char *) &cached, INT32_C(5));
	f.writeWord(tracked.off);
	f.writeWord(hold);
	f.writeWord(ready);
	f.writeWord(rejectObstacle);
	f.writeWord(actor.off);
	f.writeWord(held.off);
	f.writeWord(nearest);
	f.writeWord(destX.value);
	f.writeWord(destY.value);
	f.writeByte(destZ);
	f.writeWord(index);
	f.writeWord(failures);
	f.writeWord(path.count);
	f.write((char *) path.data, path.count * sizeof(Waypoint));
}

void Route::load(char *name)
{
	int16_t count;
	DataFile f(name, 1);
	Waypoint w;
	int16_t i;

	planCount = f.readWord();
	attempts = f.readWord();
	limit = f.readWord();
	cachedValid = f.readWord();
	f.read((char *) &cached, INT32_C(5));
	tracked = f.readWord();
	hold = f.readWord();
	ready = f.readWord();
	rejectObstacle = f.readWord();
	actor = f.readWord();
	held = f.readWord();
	nearest = f.readWord();
	destX = f.readWord();
	destY = f.readWord();
	destZ = f.readByte();
	index = f.readWord();
	failures = f.readWord();
	count = f.readWord();
	if (!path.allocate(count))
		ReportOutOfFarMemory();
	for (i = 0; i < count; i++) {
		f.read((char *) &w, INT32_C(5));
		path.set(i, &w);
	}
}
