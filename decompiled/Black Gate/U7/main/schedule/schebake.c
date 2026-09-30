/* Black Gate U7.EXE, overlay segment 290 (file offsets 0x088990 to 0x08938e, 2558 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "u7manage.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "bltshape.h"
#include "usehook.h"
#include "actutil.h"
#include "search.h"
#include "script.h"

unsigned char BakedFoodFrames[6] = { 0, 2, 3, 4, 5, 6 };

extern char far FindItemInArea(AreaSearch *, Loc, Loc, int, int, char, int);

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FACING(p) ((unsigned char) (NPC(p)->status & 7))

/* start over; the dough's type is passed where the container belongs */
inline void PutBack(objref *p)
{
	DeleteCarriedItem(&objref(658), 2);
	CUR_SCHED(p).state = 1;
}

/* Bake: make dough from flour, carry it to the oven, turn it into bread and take the bread away. */
void far RunBakeSchedule(objref *npc)
{
	objref created;
	Coord x, y;
	unsigned char z;
	int frames, frame;
	AreaSearch nearby, selected, unusedSearch;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		if (RollChance(4))
			RunUsable(0, *npc, -1);
		else
			CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(6) + 1) * 10;
		break;
	case 10:
		frames = ShapeManager_getFrameCount(&gShapeManager, 863 /* kitchen items */);
		frame = GenerateRandomIntegerInRange(frames);
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, MakeTypeFrame(863 /* kitchen items */, frame),
			RollChance(2) ? 1003 /* table */ : 1018 /* table */);
		break;
	case 11:
		CUR_SCHED(npc).state = 1;
		break;
	case 20:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 831 /* baking hearth */, -1);
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
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 831 /* baking hearth */, -1);
		break;
	case 32:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_MOVE_ITEM, 377 /* food item */, 633 /* table */);
		break;
	case 33:
		if (IsBlocked(npc)) {
			while (DeleteCarriedItem(npc, 377 /* food item */))
				;
		}
		CUR_SCHED(npc).state = 1;
		break;
	case 40:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, MakeTypeFrame(863 /* kitchen items */, 13), -1);
		break;
	case 41:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		break;
	case 42:
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		created = CreateCarriedItem(MakeTypeFrame(863 /* kitchen items */, 0), *npc);
		break;
	case 43:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 863 /* kitchen items */,
			RollChance(2) ? 1003 /* table */ : 1018 /* table */);
		break;
	case 44:
		Npc_pushSchedule(npc, WORK_GRAB_ITEM, MakeTypeFrame(863 /* kitchen items */, 0), -1);
		break;
	case 45:
		DeleteCarriedItem(npc, MakeTypeFrame(863 /* kitchen items */, 0));
		created = CreateCarriedItem(MakeTypeFrame(658 /* dough */, 0), *npc);
		CUR_SCHED(npc).state++;
		break;
	case 46:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, MakeTypeFrame(658 /* dough */, 0),
			RollChance(2) ? 1003 /* table */ : 1018 /* table */);
		break;
	case 47:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 8, SCRIPT_READY_FRAME, SCRIPT_WAIT, 5,
			SCRIPT_STAND_FRAME, SCRIPT_WAIT, 4, SCRIPT_READY_FRAME, SCRIPT_WAIT, 5, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 3,
			SCRIPT_END));
		FindItemInArea(&nearby,
			Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
			Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]),
			0x100, 658 /* dough */, 0xff, 0);
		PostScriptToItem(&objref(nearby.current.off), MakeScript(SCRIPT_WAIT, 10, SCRIPT_NEXT_FRAME, SCRIPT_WAIT, 10,
			SCRIPT_NEXT_FRAME, SCRIPT_END));
		break;
	case 48:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, MakeTypeFrame(658 /* dough */, 2), 831 /* baking hearth */);
		break;
	case 49:
		if (IsBlocked(npc))
			PutBack(npc);
		else
			CUR_SCHED(npc).state = 1;
		break;
	case 50:
		FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc), 25, 0, 831 /* baking hearth */, 0xff, 0xff);
		if (nearby.found()) {
			FindItemInArea(&selected,
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				Coord(Item_getX(nearby.current) - 1), Item_getY(nearby.current),
				0, 658 /* dough */, 0xff, 0xff);
			if (selected.found()) {
				x = Item_getX(selected.current);
				y = Item_getY(selected.current);
				z = Item_getZ(&selected.current);
				Item_delete(&selected.current);
				created = CreateCarriedItem(377 /* food item */, *npc);
				Item_setFrame(&created, BakedFoodFrames[GenerateRandomIntegerInRange(6)]);
				Item_move(&created, x, y, z);
			}
			CUR_SCHED(npc).state = 1;
		} else {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 60:
		Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		break;
	case 61:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
