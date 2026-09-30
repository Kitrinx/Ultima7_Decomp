/* Black Gate U7.EXE, overlay segment 287 (file offsets 0x085ef0 to 0x087c43, 7507 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "npcref.h"
#include "u7npc.h"
#include "u7manage.h"
#include "actutil.h"
#include "legalmov.h"
#include "sortitem.h"
#include "random.h"
#include "npcpath.h"
#include "sprite.h"
#include "actitem.h"
#include "text.h"
#include "script.h"
#include "coord.h"
#include "search.h"
#include "scheserv.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

extern int16_t DiscardedPathLength[2];

/*
 * Wait tables: serve plates and food to diners seated at the inn, set out tableware, stir the cauldron
 * or clear plates and food away.
 */
void RunWaiterSchedule(objref *npc)
{
	int8_t result;
	NPCRef diner;
	int16_t types[5] = { 944, 388, 616, 681, 628 };     /* pot, eating utensils, bottle, jar, cup */
	int16_t tables[2] = { 1003, 1018 };
	int16_t places[3] = { 1003, 1018, 872 };            /* two tables and a stove */
	objref found;
	AreaSearch unusedSearch;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		if (RollChance(3)) {
			CUR_SCHED(npc).state = 0;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		} else {
			if (RollChance(2))
				CUR_SCHED(npc).state = 10;
			else
				CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(3) + 2) * 10;
			CUR_SCHED(npc).x = 0;
		}
		break;
	case 10:
		/* serve plates first, then food */
		if (CUR_SCHED(npc).x == 0)
			CUR_SCHED(npc).x = 717 /* plate */;
		diner = FindUnservedDiner(npc, CUR_SCHED(npc).x);
		if (diner.valid()) {
			result = WalkBesideItem(npc, &diner, GetFacing(&diner));
			if (result == 0)
				CUR_SCHED(npc).state++;
		} else if (CUR_SCHED(npc).x == 717 /* plate */)
			CUR_SCHED(npc).x = 377 /* food item */;
		else
			CUR_SCHED(npc).state = 1;
		break;
	case 11:
		ContinueScheduleWalk(npc);
		break;
	case 12:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48, SCRIPT_END));
		break;
	case 13:
		if (RollChance(3)) {
			/* "Can I help thee?", "What wilt thou have?" and three more */
			SpriteManager_barkOnItem(&gSpriteManager, npc->off,
				GetGameText(1, GenerateRandomIntegerInRange(5) + 27), 0, 15, 0);
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 15, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		} else
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		break;
	case 14:
		SetDownServing(npc, CUR_SCHED(npc).x);
		PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		CUR_SCHED(npc).state = 10;
		break;
	case 20:
		CUR_SCHED(npc).y = types[GenerateRandomIntegerInRange(5)];
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, CUR_SCHED(npc).y, CUR_SCHED(npc).y == 944 /* pot */ ?
			places[GenerateRandomIntegerInRange(3)] : tables[GenerateRandomIntegerInRange(2)]);
		break;
	case 21:
		CUR_SCHED(npc).state = 1;
		break;
	case 30:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 995 /* cauldron */, -1);
		break;
	case 31:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48, SCRIPT_WAIT,
				GenerateRandomIntegerInRange(11) + 10, SCRIPT_END));
		break;
	case 32:
		CUR_SCHED(npc).state = 1;
		break;
	case 40:
		/* clear food first, then plates */
		if (CUR_SCHED(npc).x == 0)
			CUR_SCHED(npc).x = 377 /* food item */;
		found = FindItemToClear(npc, CUR_SCHED(npc).x);
		if (found.valid() && WalkBesideItem(npc, &found, 8) == 0) {
			CUR_SCHED(npc).counter = Item_getZ(&found);
			CUR_SCHED(npc).state++;
		} else if (CUR_SCHED(npc).x == 377 /* food item */)
			CUR_SCHED(npc).x = 717 /* plate */;
		else
			CUR_SCHED(npc).state = 1;
		break;
	case 41:
		ContinueScheduleWalk(npc);
		break;
	case 42:
		if (CUR_SCHED(npc).counter >= 2)
			PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48, SCRIPT_STAND_FRAME,
				SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_FACE, NPC(npc)->direction + 48, SCRIPT_STAND_FRAME,
				SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		break;
	case 43:
		/* a plate goes with the food on it */
		if (CUR_SCHED(npc).x == 717 /* plate */)
			RemoveItemAhead(npc, 377 /* food item */);
		RemoveItemAhead(npc, CUR_SCHED(npc).x);
		if (CUR_SCHED(npc).counter >= 2)
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 44:
		CUR_SCHED(npc).state = 40;
		break;
	}
}

/*
 * A diner seated at the inn within 25 cells (sitting frame 10 or 26) with no serving of this kind
 * ahead, room ahead for one, and a free cell to either side of him to serve from.
 */
NPCRef FindUnservedDiner(objref *npc, TypeFrame &serving)
{
	int16_t frame;
	uint8_t dir;
	Coord x, y;
	NPCRef diner;
	uint16_t type;
	AreaSearch npcs;
	AreaSearch served;

	if (serving.flipped())
		frame = serving.frame();
	else
		frame = 0xff;
	type = serving.type();
	FindItemInArea(&npcs, Coord(Item_getX(*npc) - 25), Coord(Item_getY(*npc) - 25),
		Coord(Item_getX(*npc) + 25), Coord(Item_getY(*npc) + 25), 4, -1, 0xff, 0xff);
	while (npcs.found()) {
		diner = npcs.current;
		x = Item_getX(npcs.current);
		y = Item_getY(npcs.current);
		dir = GetFacing(&diner);
		if (NPC(&diner)->workType == WORK_EAT_AT_INN &&
				(npcs.current.frame() == 10 || npcs.current.frame() == 26)) {
			FindItemInArea(&served, Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
				Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]), 0, serving.type(), 0xff, frame);
			if (!served.found()) {
				if (type == 717 /* plate */ && CanWalkAt(Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
						Item_getZ(&diner) + 2, type) ||
					type == 377 /* food item */ && CanWalkAt(Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
						Item_getZ(&diner) + 3, 377 /* food item */)) {
					if (CanWalkAt(Coord(x + DirDeltaX[(dir + 6) & 7]), Coord(y + DirDeltaY[(dir + 6) & 7]),
							Item_getZ(npc), npc->ptr()->typeFrame))
						return NPCRef(npcs.current);
					if (CanWalkAt(Coord(x + DirDeltaX[(dir + 10) & 7]), Coord(y + DirDeltaY[(dir + 10) & 7]),
							Item_getZ(npc), npc->ptr()->typeFrame))
						return NPCRef(npcs.current);
				}
			}
		}
		FindItem(&npcs);
	}
	return NPCRef(npcs.current);
}

/*
 * Walk to the cell to the left or right of target as it faces dir, or with dir 8 to any free side.
 * Returns what StartPath does, 0 also when already beside it and 2 when no cell is free.
 */
int8_t WalkBesideItem(objref *npc, objref *target, uint8_t dir)
{
	int8_t result;
	Coord x, y;
	int16_t i;

	x = Item_getX(*target);
	y = Item_getY(*target);
	if (Item_greatestDeltaToItem(*npc, *target) == 1 && (Item_getX(*npc) == x || Item_getY(*npc) == y)) {
		if (Item_getX(*npc) == x)
			dir = Item_getY(*npc) > y ? 0 : 4;
		else
			dir = Item_getX(*npc) > x ? 6 : 2;
		return 0;
	}
	if (dir == 8) {
		for (i = 2; i < 10; i += 2) {
			if (CanWalkAt(x + DirDeltaX[i & 7], y + DirDeltaY[i & 7], Item_getZ(npc), npc->ptr()->typeFrame)) {
				x += DirDeltaX[i & 7];
				y += DirDeltaY[i & 7];
				dir = (i + 4) & 7;
				break;
			}
		}
		if (i == 10)
			return 2;
	} else {
		if (CanWalkAt(x + DirDeltaX[(dir + 6) & 7], y + DirDeltaY[(dir + 6) & 7],
				Item_getZ(npc), npc->ptr()->typeFrame)) {
			NPC(npc)->scheduleValue = 0;
			x += DirDeltaX[(dir + 6) & 7];
			y += DirDeltaY[(dir + 6) & 7];
		} else if (CanWalkAt(x + DirDeltaX[(dir + 2) & 7], y + DirDeltaY[(dir + 2) & 7],
				Item_getZ(npc), npc->ptr()->typeFrame)) {
			NPC(npc)->scheduleValue = 1;
			x += DirDeltaX[(dir + 2) & 7];
			y += DirDeltaY[(dir + 2) & 7];
		} else
			return 2;
	}
	result = StartPath(*npc, x, y, 0, 100, DiscardedPathLength, 0);
	if (result == 0)
		NPC(npc)->direction = dir;
	return result;
}

/* Set a new serving on the table ahead of the diner beside the waiter, unless one is there. */
void SetDownServing(objref *npc, TypeFrame &serving)
{
	Coord x, y;
	objref dish;
	int16_t dropX, dropY, dropZ, ownX, ownY, ownZ;
	AreaSearch existing;
	int16_t frame;

	if (serving.type() == 717 /* plate */)
		frame = PlateFrames[GenerateRandomIntegerInRange(2)];
	else if (serving.type() == 377 /* food item */)
		frame = FoodFrames[GenerateRandomIntegerInRange(17)];
	else if (serving.flipped())
		frame = serving.frame();
	else
		frame = 0xff;
	x = Item_getX(*npc);
	y = Item_getY(*npc);
	x += DirDeltaX[(GetFacing(npc) + (NPC(npc)->scheduleValue ? 7 : 1)) & 7];
	y += DirDeltaY[(GetFacing(npc) + (NPC(npc)->scheduleValue ? 7 : 1)) & 7];
	FindItemInArea(&existing, x, y, x, y, 0, serving.type(), 0xff, 0xff);
	if (existing.found())
		return;
	/* nothing reads these */
	dropX = x;
	dropY = y;
	dropZ = Item_getZ(npc) + (serving.type() == 717 /* plate */ ? 0 : 1) + 2;
	ownX = Item_getX(*npc);
	ownY = Item_getY(*npc);
	ownZ = Item_getZ(npc);
	/* a plate sits on the table top, food on the plate */
	if (CanWalkAt(x, y, Item_getZ(npc) + (serving.type() == 717 /* plate */ ? 0 : 1) + 2, serving.type())) {
		CreateItem(&dish, serving.type(), x, y, Item_getZ(npc) + (serving.type() == 717 /* plate */ ? 0 : 1) + 2);
		Item_setTemporary(&dish);
		Item_clearOkayToTake(&dish);
		if (frame != 0xff)
			Item_setFrame(&dish, ClampShapeFrame(dish.type(), frame));
	}
}

/* An item of this kind within 25 cells with no one beside it but the waiter and a free side to reach. */
objref FindItemToClear(objref *npc, TypeFrame &wanted)
{
	int16_t frame;
	Coord x, y;
	AreaSearch items;
	AreaSearch beside;
	int16_t dir;

	if (wanted.flipped())
		frame = wanted.frame();
	else
		frame = 0xff;
	FindItemInArea(&items, Coord(Item_getX(*npc) - 25), Coord(Item_getY(*npc) - 25),
		Coord(Item_getX(*npc) + 25), Coord(Item_getY(*npc) + 25), 0, wanted.type(), 0xff, frame);
	while (items.found()) {
		x = Item_getX(items.current);
		y = Item_getY(items.current);
		for (dir = 0; dir < 8; dir += 2) {
			FindItemInArea(&beside, x + DirDeltaX[dir], y + DirDeltaY[dir],
				x + DirDeltaX[dir], y + DirDeltaY[dir], 4, -1, 0xff, 0xff);
			if (npc->off == beside.current.off)
				FindItem(&beside);
			if (beside.found())
				break;
		}
		if (dir == 8) {
			for (dir = 0; dir < 8; dir += 2)
				if (CanWalkAt(Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
						Item_getZ(npc), npc->ptr()->typeFrame))
					return objref(items.current.off);
		}
		FindItem(&items);
	}
	return objref(items.current.off);
}

/* Take away an item of this kind from the cell the waiter faces. */
void RemoveItemAhead(objref *npc, TypeFrame &wanted)
{
	AreaSearch ahead;
	int16_t frame;

	if (wanted.flipped())
		frame = wanted.frame();
	else
		frame = 0xff;
	FindItemInArea(&ahead, Coord(Item_getX(*npc) + DirDeltaX[GetFacing(npc)]),
		Coord(Item_getY(*npc) + DirDeltaY[GetFacing(npc)]),
		Coord(Item_getX(*npc) + DirDeltaX[GetFacing(npc)]),
		Coord(Item_getY(*npc) + DirDeltaY[GetFacing(npc)]), 0, wanted.type(), 0xff, frame);
	if (ahead.found())
		Item_delete(&objref(ahead.current.off));
}
