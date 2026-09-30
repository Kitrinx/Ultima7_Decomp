/* Black Gate U7.EXE, resident segment 37 (file offsets 0x01ab88 to 0x01bf50, 5064 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "typefram.h"
#include "activity.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "objref.h"
#include "combat.h"
#include "crime.h"
#include "sprite.h"
#include "text.h"
#include "sounds.h"
#include "usehook.h"
#include "u7npc.h"
#include "npcpath.h"
#include "type.h"
#include "coord.h"
#include "npcref.h"
#include "legalmov.h"
#include "partymov.h"
#include "search.h"
#include "movepath.h"
#include "route.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ITEM(r) ((ItemRecord far *)MK_FP(ItemBufferSegment, (r).off))
#define TYPE(r) (ITEM(r)->typeFrame & 0x3ff)
#define FRAME(r) ((ITEM(r)->typeFrame & 0x7c00) >> 10)
#define IS_NPC(r) ((char)((ItemTypeClassFlags[gItemTypeInfo[TYPE(r)].typeClass] & CLASS_NPC) != 0))

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);

inline void SetItemTypeBits(objref *ref, int type)
{
	unsigned far &bits = ITEM(*ref)->typeFrame;

	bits = (type & 0x3ff) | (bits & 0xfc00);
}

inline void SetItemFrameBits(objref *ref, int frame)
{
	unsigned far &bits = ITEM(*ref)->typeFrame;

	bits = (bits & 0x83ff) | ((frame << 10) & 0x7c00);
}

char PathFileName[] = "PATH.DAT";
NPCRef RouteOwners[15];
Route *NPCRoutes[15] = { 0 };
Route *Autoroute = 0;
Waypoint WaypointScratch;
int DiscardedPathLength[2]; /* callers pass its address as a path length */
int ArrivalUsecode, ArrivalItem, ArrivalEvent;
int FailureUsecode, FailureItem, FailureEvent;

extern "C" void far *_fmemcpy(void far *, const void far *, unsigned);
extern unsigned char far Item_getQualityFlags(objref *);
extern unsigned GetCursorLength();

extern unsigned char far IsSentient(NPCRef &npc);
extern unsigned char far Item_detach(objref *ref);
extern int far PlaceItem(objref *, Loc, Loc, int);
extern int far PlaceItem(objref *, Loc, Loc, char);
extern "C" int far Item_greatestDeltaToItem(objref &, objref *);
extern void far Item_move(objref *, unsigned char, int);

extern unsigned char far IsPointInItem(ItemId id, Coord x, Coord y, int z);

extern "C" void far RunItemScript(unsigned char *, NPCRef);

void far StepAvatar(char direction, int count)
{
	int i;

	if (Autoroute != 0)
		StopAutoroute(2);
	if (!((unsigned char)Item_getQualityFlags(&AvatarRef) & 0x20)) {
		if (CombatGroups.battleMusic | AvatarInCombat) {
			while ((char)(CombatGroups.movePoints[Item_getNpcNumber((objref *)&NPCRef(AvatarRef))] >= 16)) {
				CombatGroups.movePoints[Item_getNpcNumber((objref *)&NPCRef(AvatarRef))] -= 16;
				MoveParty(direction);
				if (GetCursorLength() == 1)
					return;
			}
		} else {
			for (i = 0; i < count; ++i)
				MoveParty(direction);
		}
	}
}

Waypoint *PathStore::getWaypoint(int index)
{
	_fmemcpy(&WaypointScratch, data + index, sizeof(Waypoint));
	return &WaypointScratch;
}

unsigned char far StartAutoroute(Coord x, Coord y, char z, int silent, int callback, int argument, int event)
{
	int length;

	FailureUsecode = 0;
	FailureItem = 0;
	FailureEvent = 0;
	ArrivalUsecode = callback;
	ArrivalItem = argument;
	ArrivalEvent = event;
	if (Autoroute != 0) {
		delete Autoroute;
		Autoroute = 0;
	}
	Autoroute = new Route(objref(AvatarRef.off), x, y, z, 1, 0, -1, &length, 0);
	if (Autoroute == 0) {
		if (silent == 0)
			ReportNoCanDo(7);
		return 0;
	}
	if (!Autoroute->ready) {
		if (silent == 0)
			ReportNoCanDo(7);
		delete Autoroute;
		Autoroute = 0;
		return 0;
	}
	return 1;
}

char far ContinueAutoroute()
{
	char status;

	if (Autoroute == 0 || (unsigned char)(Item_getQualityFlags(&AvatarRef) & 0x20))
		return 0;
	if (CombatGroups.battleMusic | AvatarInCombat) {
		while ((char)(CombatGroups.movePoints[Item_getNpcNumber((objref *)&NPCRef(AvatarRef))] >= 16)) {
			status = Autoroute->walk(1);
			CombatGroups.movePoints[Item_getNpcNumber((objref *)&NPCRef(AvatarRef))] -= 16;
		}
	} else {
		status = Autoroute->walk(1);
	}
	if (status == 0) {
		if (ArrivalUsecode != 0)
			RunUsable(ArrivalEvent, ArrivalItem, ArrivalUsecode);
		StopAutoroute(status);
		return 1;
	} else if (status == 2) {
		StopAutoroute(status);
		return 0;
	}
}

void far SetRouteFailureUsecode(int callback, int argument, int event)
{
	FailureUsecode = callback;
	FailureItem = argument;
	FailureEvent = event;
}

void far StopAutoroute(unsigned char reason)
{
	if (Autoroute != 0) {
		ArrivalUsecode = 0;
		ArrivalItem = 0;
		ArrivalEvent = 0;
		delete Autoroute;
		Autoroute = 0;
		if (FailureUsecode != 0) {
			if (reason == 2) {
				RunUsable(FailureEvent, FailureItem, FailureUsecode);
				FailureUsecode = 0;
				FailureItem = 0;
				FailureEvent = 0;
			}
		}
	}
}

Walker::Walker()
{
	destX = destY = 0;
	destZ = 0;
}

Walker::Walker(objref actor, Coord x, Coord y, char z, char avoid)
	: MovementState(actor)
{
	destX = x;
	destY = y;
	destZ = z;
	rejectObstacle = avoid;
}

char Walker::chooseStep()
{
	char dir, first, second;
	int dx = GetDelta(destX, x);
	int dy = GetDelta(destY, y);
	int sx = GetSign(dx);
	int sy = GetSign(dy);

	dir = DirectionBySign[sx + 1][sy + 1];
	if (dir == -1)
		return dir;
	if (tryStep(dir, rejectObstacle))
		return dir;
	if (sx != 0 && sy != 0) {
		if ((dx < 0 ? -dx : dx) >= (dy < 0 ? -dy : dy)) {
			first = DirectionBySign[sx + 1][1];
			second = DirectionBySign[1][sy + 1];
		} else {
			first = DirectionBySign[1][sy + 1];
			second = DirectionBySign[sx + 1][1];
		}
		if (tryStep(first, rejectObstacle))
			return first;
		if (tryStep(second, rejectObstacle))
			return second;
	}
	return -1;
}

void Walker::setTarget(Coord x, Coord y, char z)
{
	destX = x;
	destY = y;
	destZ = z;
}

unsigned char Walker::isAtTarget()
{
	return destX == x && destY == y && destZ == z;
}

int Walker::getDistance()
{
	return MAX(GetMagnitude(GetDelta(x, destX)), GetMagnitude(GetDelta(y, destY)));
}

char Walker::getDirection()
{
	int sx = GetSign(GetDelta(destX, x));
	int sy = GetSign(GetDelta(destY, y));

	return DirectionBySign[sx + 1][sy + 1];
}

char far WalkNPCRoute(objref npc, int steps)
{
	char status;

	for (int i = 0; i < 15; ++i) {
		if (RouteOwners[i] == objref(npc.off) && RouteOwners[i].valid()) {
			if ((unsigned char)(Item_getQualityFlags(&npc) & 0x20))
				return 1;
			status = NPCRoutes[i]->walk(steps);
			if (status == 0 || status == 2)
				EndPathSlot(i);
			return status;
		}
	}
	return 0;
}

Route::~Route()
{
	closeHeldDoor();
}

/* Swaps the held door between its open and shut types; a door whose type changes orientation also
 * moves 3 cells. A locked door (state 2 or 3), or an NPC that cannot open doors, leaves it as it is;
 * the avatar then barks "Locked". */
void Route::toggleDoor()
{
	int newType;
	int x, y, z, moveX, moveY, frame;
	char unused = 0;
	unsigned state, newState, type;

	type = TYPE(held);
	x = Item_getX(held);
	y = Item_getY(held);
	z = Item_getZ(&held);
	moveX = moveY = 0;
	frame = FRAME(held);
	state = frame & 3;
	if ((state != 1 && state != 0) || (IS_NPC(actor) && !IsSentient(actor))) {
		if ((unsigned char)Item_isAvatar((objref *)&NPCRef(actor)))
			SpriteManager_barkOnItem(&gSpriteManager, actor, "Locked", 5, 15, 0);
		held.off = 0;
		return;
	}
	switch (type) {
	case 376: newType = 270; break;     /* door */
	case 270: newType = 376; break;     /* door */
	case 433:                           /* door */
		if (state == 1)
			moveX = 3;
		else
			moveY = -3;
		newType = 432;
		break;
	case 432:                           /* door */
		if (state == 1)
			moveY = 3;
		else
			moveX = -3;
		newType = 433;
		break;
	case 392: newType = 225; break;     /* Abbey door */
	case 225: newType = 392; break;     /* Abbey door */
	case 250:                           /* Abbey door */
		if (state == 1)
			moveX = 3;
		else
			moveY = -3;
		newType = 246;
		break;
	case 246:                           /* Abbey door */
		if (state == 1)
			moveY = 3;
		else
			moveX = -3;
		newType = 250;
		break;
	default:
		held.off = 0;
	}
	char sound;
	if (state == 1) {
		newState = 0;
		sound = 31;
	} else {
		newState = 1;
		sound = 30;
	}
	Item_detach(&held);
	SetItemTypeBits(&held, newType);
	SetItemFrameBits(&held, frame - state + newState);
	PlaceItem(&held, Coord(x + moveX), Coord(y + moveY), z);
	PlaySoundAtItem(sound, held);
}

/* Opens a newly held door and measures the actor's distance to it. */
void Route::openHeldDoor()
{
	objref from;

	if (!tracked.valid())
		tracked = held;
	else if (tracked != held)
		tracked = held;
	else
		return;
	toggleDoor();
	from = actor;
	nearest = Item_greatestDeltaToItem(held, &from);
}

/* Puts the held door back unless the route keeps it. */
void Route::closeHeldDoor()
{
	if (held.valid() && !hold) {
		toggleDoor();
		held.off = 0;
		nearest = 999;
	}
}

/* Tracks the nearest the actor came to the held door; shuts it once the actor is more than 2 further. */
void Route::checkDoorDistance()
{
	objref from;
	int distance;

	if (held.valid()) {
		from = actor;
		distance = Item_greatestDeltaToItem(held, &from);
		if (nearest > distance)
			nearest = distance;
		else if (nearest + 2 < distance)
			closeHeldDoor();
	}
}

/* Finds the first door beside the actor standing at x, y, z. */
objref Route::findDoor(Coord x, Coord y, char z, char direction)
{
	Coord left, right, top, bottom;

	left = Coord(x - GetFootprintX(ITEM(actor)->asTypeFrame()));
	top = Coord(y - GetFootprintY(ITEM(actor)->asTypeFrame()));
	right = x;
	bottom = y;
	objref result(0);
	AreaSearch items;
	FindItemInArea(&items, left, top, Coord(left + 7), Coord(top + 7), 0, -1, 255, 255);
	while (items.current.valid()) {
		if ((unsigned char)gItemTypeInfo[ITEM(items.current)->typeFrame & 0x3ff].door) {
			for (Coord cx = left; cx <= right; ++cx) {
				for (Coord cy = top; cy <= bottom; ++cy) {
					if (IsPointInItem(items.current, cx, cy, z)) {
						result.off = items.current.off;
						goto found;
					}
				}
			}
		}
		FindItem(&items);
	}
found:
	return result;
}

/* Walks the actor up to `steps` cells along its path. Returns 0 on arrival, 1 while still under way
 * and 2 when the route has failed. */
char Route::walk(int steps)
{
	Waypoint point;
	char direction = -1, result = 1;
	int length;
	unsigned char moved;
	int maxSteps = 30;
	unsigned char isAvatar = Item_isAvatar((objref *)&NPCRef(actor));
	Walker stepper(actor, destX, destY, destZ, 0);

	for (int i = 0; i < steps; ++i) {
		stepper.setPosition(Item_getX(actor), Item_getY(actor), Item_getZ(&actor));
		stepper.setItem(Item_getX(actor), Item_getY(actor), Item_getZ(&actor));
		if (failures > 3) {
			failures = 0;
			if (plan(0, 1, 0, -1, &length, 0)) {
				index = 0;
			} else {
				result = 2;
				break;
			}
		}
		stepper.setTarget(destX, destY, destZ);
		if (stepper.isAtTarget()) {
			direction = -1;
			result = 0;
			break;
		}
		point = *path.getWaypoint(index);
		stepper.setTarget(point.x, point.y, point.z);
		if (stepper.isAtTarget()) {
			++index;
			point = *path.getWaypoint(index);
			stepper.setTarget(point.x, point.y, point.z);
		}
		if (cachedValid) {
			stepper.rejectObstacle = 1;
			if (cached.x == stepper.x && cached.y == stepper.y && cached.z == stepper.z)
				cachedValid = 0;
		} else {
			stepper.rejectObstacle = 0;
		}
		direction = stepper.chooseStep();
		if (limit + limit / 2 < attempts++ && limit > 10) {
			result = 2;
			break;
		}
		if (direction == -1) {
			++failures;
			direction = -1;
			result = 1;
			break;
		}
		if (stepper.blocked()) {
			unsigned char found = 0;
			int j;
			for (j = index; j < path.count; ++j) {
				if (CanItemMoveTo(path.getWaypoint(j)->x, path.getWaypoint(j)->y,
					path.getWaypoint(j)->z, actor)) {
					_fmemcpy(&cached, path.data + j, sizeof(Waypoint));
					cachedValid = 1;
					found = 1;
					break;
				}
			}
			if (!found) {
				result = 2;
				break;
			}
			if (plan(1, 1, 1, maxSteps, &length, j)) {
				direction = -1;
				result = 1;
				index = 0;
				break;
			}
			held = findDoor(stepper.x, stepper.y, stepper.z, direction);
			if (!held.valid()) {
				result = 2;
				break;
			}
			openHeldDoor();
			if (!held.valid()) {
				result = 2;
				break;
			}
			if (!hold) {
				if (plan(1, 1, 1, maxSteps, &length, j)) {
					direction = -1;
					result = 1;
					index = 0;
					break;
				} else {
					result = 2;
					break;
				}
			}
		}
		if (result != 2 && isAvatar && GetNpcBufferForIbo(&AvatarRef)->workType == WORK_FOLLOW_AVT) {
			MoveParty(direction);
		} else if (isAvatar && GetNpcBufferForIbo(&AvatarRef)->workType != WORK_FOLLOW_AVT) {
			Item_move(&actor, direction, stepper.zChange);
			if (direction == -1)
				moved = 0;
			else
				moved = 1;
			if (!(Item_getQualityFlags(&actor) & 0x20)) {
				unsigned char script[4];
				script[2] = moved;
				script[3] = direction;
				RunItemScript(script, actor);
			}
		} else if (!isAvatar) {
			Item_move(&actor, direction, stepper.zChange);
		}
		checkDoorDistance();
	}
	if (IS_NPC(actor)) {
		if (IsNpcUnconscious((objref *)&NPCRef(actor)))
			result = 2;
	}
	if (result != 2 && !isAvatar) {
		if (direction == -1)
			moved = 0;
		else
			moved = 1;
		if (!(Item_getQualityFlags(&actor) & 0x20)) {
			unsigned char script[4];
			script[2] = moved;
			script[3] = direction;
			RunItemScript(script, actor);
		}
	}
	return result;
}
