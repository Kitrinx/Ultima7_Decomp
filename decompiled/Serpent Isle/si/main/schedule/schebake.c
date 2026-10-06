/* Serpent Isle SI.EXE, overlay segment 274 (file offsets 0x076c00 to 0x0776d9, 2777 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "usehook.h"
#include "actutil.h"
#include "search.h"
#include "script.h"

unsigned char BakedFoodFrames[6] = { 0, 2, 3, 4, 5, 6 };

extern char far FindItemInArea(AreaSearch *, Loc, Loc, int, int, char, int);

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FACING(p) ((unsigned char) (NPC(p)->status & 7))

/*
 * Bake: make dough from flour, carry it to the oven, turn it into bread and take the bread away. The
 * dough is a kitchen item, frames 16 to 18 as it rises.
 */
void far RunBakeSchedule(objref *npc)
{
	objref created;
	Coord x, y;
	unsigned char z;
	int frame;
	AreaSearch nearby, selected, unusedSearch;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		if (RollChance(4))
			RunUsable(0, *npc, -1);
		else
			CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(7) + 1) * 10;
		break;
	case 10:
		frame = KitchenItemFrames[GenerateRandomIntegerInRange(5)];
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, MakeTypeFrame(863 /* kitchen items */, frame),
			RollChance(2) ? 1003 /* table */ : 1018 /* table */, 0);
		break;
	case 11:
		CUR_SCHED(npc).state = 1;
		break;
	case 20:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 831 /* baking hearth */, -1, 0);
		break;
	case 21:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(9) + 12, SCRIPT_END));
		break;
	case 22:
		CUR_SCHED(npc).state = 1;
		break;
	case 30:
		FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc), 25, 0, 831 /* baking hearth */, 0xff, 0xff);
		if (nearby.found()) {
			FindItemInArea(&nearby,
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				0, 377 /* food item */, 0xff, 0xff);
			if (nearby.found())
				CUR_SCHED(npc).state++;
			else
				CUR_SCHED(npc).state = 1;
		} else {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 31:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 831 /* baking hearth */, -1, 0);
		break;
	case 32:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_MOVE_ITEM, 377 /* food item */,
				RollChance(2) ? 633 /* table */ : 1003 /* table */, 0);
		break;
	case 33:
		CUR_SCHED(npc).state = 1;
		break;
	case 40:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, MakeTypeFrame(863 /* kitchen items */, 13), -1, 0);
		break;
	case 41:
		/* no flour within reach: bring some */
		if (IsBlocked(npc)) {
			created = CreateCarriedItem(MakeTypeFrame(863 /* kitchen items */, 13), *npc);
			Npc_pushSchedule(npc, WORK_DROP_ITEM, MakeTypeFrame(863 /* kitchen items */, 13), -1, 0);
		} else
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 42:
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		created = CreateCarriedItem(MakeTypeFrame(863 /* kitchen items */, 0), *npc);
		break;
	case 43:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, MakeTypeFrame(863 /* kitchen items */, 0),
			RollChance(2) ? 1003 /* table */ : 1018 /* table */, 0);
		break;
	case 44:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, MakeTypeFrame(863 /* kitchen items */, 0), -1, 0);
		break;
	case 45:
		while (DeleteCarriedItem(npc, 863 /* kitchen items */))
			;
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else {
			created = CreateCarriedItem(MakeTypeFrame(863 /* kitchen items */, 16 /* dough */), *npc);
			CUR_SCHED(npc).state++;
		}
		break;
	case 46:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, MakeTypeFrame(863 /* kitchen items */, 16 /* dough */),
			RollChance(2) ? 1003 /* table */ : 1018 /* table */, 0);
		break;
	case 47:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 8, SCRIPT_READY_FRAME, SCRIPT_WAIT, 5,
			SCRIPT_STAND_FRAME, SCRIPT_WAIT, 4, SCRIPT_READY_FRAME, SCRIPT_WAIT, 5, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 3,
			SCRIPT_END));
		FindItemInArea(&nearby,
			Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
			Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]),
			0x100, 863 /* kitchen items */, 0xff, 16 /* dough */);
		PostScriptToItem(&objref(nearby.current.off), MakeScript(SCRIPT_WAIT, 10, SCRIPT_NEXT_FRAME, SCRIPT_WAIT, 10,
			SCRIPT_NEXT_FRAME, SCRIPT_END));
		break;
	case 48:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, MakeTypeFrame(863 /* kitchen items */, 18 /* risen dough */),
			831 /* baking hearth */, 0);
		break;
	case 49:
		CUR_SCHED(npc).state = 1;
		break;
	case 50:
		FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc), 25, 0, 831 /* baking hearth */, 0xff, 0xff);
		if (nearby.found()) {
			FindItemInArea(&selected,
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				0, 863 /* kitchen items */, 0xff, 0xff);
			if (selected.found()) {
				x = Item_getX(selected.current);
				y = Item_getY(selected.current);
				z = Item_getZ(&selected.current);
				Item_delete(&selected.current);
				created = CreateCarriedItem(377 /* food item */, *npc);
				Item_setFrame(&created, BakedFoodFrames[GenerateRandomIntegerInRange(6)]);
				Item_move(&created, x, y, z);
				CUR_SCHED(npc).state = 1;
			} else
				CUR_SCHED(npc).state = 1;
		} else {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 60:
		Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1, 0);
		break;
	case 61:
		CUR_SCHED(npc).state = 1;
		break;
	case 70:
		/* tidy up: drop whatever kitchen items and food are still carried */
		while (DeleteCarriedItem(npc, 863 /* kitchen items */))
			;
		while (DeleteCarriedItem(npc, 377 /* food item */))
			;
		CUR_SCHED(npc).state = 1;
		break;
	}
}
