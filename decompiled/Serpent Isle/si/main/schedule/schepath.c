/* Serpent Isle SI.EXE, overlay segment 281 (file offsets 0x07a260 to 0x07ae88, 3112 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "coord.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "item.h"
#include "u7npc.h"
#include "npcref.h"
#include "search.h"
#include "npcpath.h"
#include "actitem.h"
#include "combatai.h"
#include "combmode.h"
#include "usehook.h"

/* the schedule of an NPC walking somewhere for usecode */
#define WORK_PATHFIND   24

/* the usable's event when the walk is given up */
#define EVENT_PATH_FAILED   14

#define NPC(p)          GetNpcBufferForIbo(p)
#define CUR_SCHED(p)    (NPC(p)->schedules[NPC(p)->currentSchedule])

extern int DiscardedPathLength;

/* the NPCs walking for usecode, and how often each has tried */
int PathfindNpcs[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
unsigned char PathfindTries[10] = { 0 };

char FindPathfinder(objref npc);
void AddPathfinder(objref npc);

/* Forget the walk and run its usable for event; an NPC still walking then stands. */
#define END_WALK(event) \
	PathfindNpcs[slot] = -1; \
	PathfindTries[slot] = 0; \
	RunUsable(event, item, NPC(npc)->scheduleToggle); \
	if (NPC(npc)->workType != WORK_PATHFIND) \
		return; \
	Npc_setSchedule(npc, WORK_STAND); \
	CUR_SCHED(npc).state = 1; \
	return

/* One step of the walk schedule: head for the place in the schedule, or the avatar when it is 0, in
 * ever shorter hops; at the end run the usable with the event the walk was started with. */
void RunPathfindSchedule(objref *npc)
{
	int step = 14;
	AreaSearch items;
	Coord x, y, z;
	Coord cx, cy;
	char busy;
	char result;
	int spotZ;
	int item = NPC(npc)->scheduleValue;
	char slot = FindPathfinder(*npc);

	if (++PathfindTries[slot] > 100) {
		END_WALK(EVENT_PATH_FAILED);
	} else {
		switch (CUR_SCHED(npc).state) {
		case 1:
			busy = ContinueScheduleWalk(npc);
			if (busy)
				return;
			if (CUR_SCHED(npc).x + CUR_SCHED(npc).y + CUR_SCHED(npc).counter == 0u) {
				x = Item_getX(AvatarRef);
				y = Item_getY(AvatarRef);
				z = Item_getZ(&AvatarRef);
			} else {
				x = CUR_SCHED(npc).x;
				y = CUR_SCHED(npc).y;
				z = CUR_SCHED(npc).counter;
			}
			if (Item_getX(*npc) == x && Item_getY(*npc) == y) {
				END_WALK(NPC(npc)->iVr[1] & 0xff);
			}
			if (StartPath(*npc, x, y, z, 150, &DiscardedPathLength, 0) == 0)
				return;
			/* walk toward the place in ever shorter hops */
		next:
			cx = Item_getX(*npc);
			cy = Item_getY(*npc);
			if (cx != x) {
				if (cx < x) {
					cx += step;
					if (cx > x)
						cx = x;
				} else {
					cx -= step;
					if (cx < x)
						cx = x;
				}
			}
			if (cy != y) {
				if (cy < y) {
					cy += step;
					if (cy > y)
						cy = y;
				} else {
					cy -= step;
					if (cy < y)
						cy = y;
				}
			}
			result = StartPath(*npc, cx, cy, z, 150, &DiscardedPathLength, 0);
			if (result != 2)
				return;
			if (step >= 6) {
				step -= 4;
				goto next;
			}
			{
				/* stuck far from the avatar, where nobody sees: jump there */
				if (GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef), x, y, 0) < 24 ||
					GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef), Item_getZ(&AvatarRef), Item_getX(*npc),
						Item_getY(*npc), Item_getZ(npc)) < 24)
					goto close;
				Item_move(npc, CellCoord(x), CellCoord(y));
				return;
			close:
				if (NPC(npc)->iVr[1] & 0x100) {
					if (GetDistance(Item_getX(*npc), Item_getY(*npc), Item_getZ(npc), x, y, 0) <= 2) {
						END_WALK(NPC(npc)->iVr[1] & 0xff);
					} else {
						FindItemInArea(&items, cx, cy, cx, cy, 0x20, -1, 255, 255);
						if (items.found() && FindSpotNextToItem(npc, items.current, &cx, &cy, &spotZ)) {
							result = StartPath(*npc, cx, cy, spotZ, 150, &DiscardedPathLength, 0);
							if (result == 0 || result == 1)
								return;
						}
						END_WALK(EVENT_PATH_FAILED);
					}
				} else {
					END_WALK(EVENT_PATH_FAILED);
				}
			}
			break;
		case 2:
			CUR_SCHED(npc).state = 1;
			break;
		default:
			return;
		}
	}
}

/* the NPC's place among the walkers, made when it has none */
char FindPathfinder(objref npc)
{
	char i;

	for (i = 0; i < 10; i++)
		if (Item_getNpcNumber(&npc) == PathfindNpcs[i])
			return i;
	AddPathfinder(npc);
	for (i = 0; i < 10; i++)
		if (Item_getNpcNumber(&npc) == PathfindNpcs[i])
			return i;
	PathfindNpcs[9] = Item_getNpcNumber(&npc);
	PathfindTries[9] = 0;
	return 9;
}

/* Note an NPC as walking, dropping walkers that stopped; the last place goes when all are taken. */
void AddPathfinder(objref npc)
{
	objref other;
	unsigned char added = 0;
	char i;

	for (i = 0; i < 10; i++) {
		if (PathfindNpcs[i] != -1) {
			GetNpcIbo(&other, PathfindNpcs[i]);
			if (NPC(&other)->workType != WORK_PATHFIND || Item_getNpcNumber(&other) == Item_getNpcNumber(&npc)) {
				PathfindNpcs[i] = -1;
				PathfindTries[i] = 0;
			}
		}
		if (PathfindNpcs[i] == -1 && !added) {
			PathfindNpcs[i] = Item_getNpcNumber(&npc);
			added = 1;
		}
	}
	if (!added) {
		PathfindNpcs[9] = Item_getNpcNumber(&npc);
		PathfindTries[9] = 0;
	}
}

/* Send an NPC to a place, cured of what would stop it, to run usable func on item with event when
 * there; with flag bit 0 set it may end beside an item in the way. */
void far PathfindNPC(int number, Coord x, Coord y, char z, unsigned char event, int item, int func,
	unsigned char flag)
{
	int spot;
	objref npc = number;

	AddPathfinder(npc);
	Npc_setSchedule(&npc, WORK_PATHFIND);
	Npc_pushSchedule(&npc, WORK_PATHFIND, x, y, z);
	CUR_SCHED(&npc).state = 1;
	NPC(&npc)->scheduleValue = item;
	NPC(&npc)->scheduleToggle = func;
	NPC(&npc)->changeFlags(NPC_ASLEEP, 0);
	NPC(&npc)->changeFlags(NPC_CHARMED, 0);
	NPC(&npc)->changeFlags(NPC_CURSED, 0);
	NPC(&npc)->changeFlags(NPC_PARALYZED, 0);
	NPC(&npc)->changeFlags(NPC_PROTECTED, 0);
	spot = flag << 8;
	spot |= event;
	SetNpcSecondSpotY(&npc, Coord(spot));
}
