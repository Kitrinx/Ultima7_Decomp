/* Black Gate U7.EXE, overlay segment 312 (file offsets 0x090840 to 0x090e6e, 1582 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "gtimer.h"
#include "sprite.h"
#include "npcpath.h"
#include "random.h"
#include "script.h"
#include "actitem.h"
#include "usehook.h"
#include "search.h"
#include "text.h"

extern int16_t DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Walk to the nearest shutters in the wrong state for the hour and use them: open by day, closed by
 * night. With an argument of 1, walk back afterwards.
 */
void DoWorkCheckShutters(objref *npc)
{
	Coord x, y;
	int16_t type, otherType;
	int16_t workType;
	int16_t facing;
	objref shutters;
	AreaSearch first, second, nearby;
	int16_t radius;

	switch (CUR_SCHED(npc).state) {
	case 0:
		/* an argument of 1 asks to come back here afterwards */
		if (CUR_SCHED(npc).x == 1) {
			CUR_SCHED(npc).x = Item_getX(*npc);
			CUR_SCHED(npc).y = Item_getY(*npc);
		}
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		if (IsDaytime()) {
			type = 291 /* closed shutters */;
			otherType = 290 /* closed shutters */;
		} else {
			type = 372 /* open shutters */;
			otherType = 322 /* open shutters */;
		}
		workType = NPC(npc)->workType;
		switch (workType) {
		case WORK_PREACH:
			radius = 12;
			break;
		case WORK_SLEEP:
		case WORK_WAITER:
			radius = 10;
			break;
		default:
			radius = 15;
		}
		FindNearestItem(&first, x, y, radius, 0, type, 0xff, 0xff);
		FindNearestItem(&second, x, y, radius, 0, otherType, 0xff, 0xff);
		if (first.found() && !second.found())
			shutters = objref(first);
		else if (!first.found() && second.found())
			shutters = objref(second);
		else if (!first.found() && !second.found()) {
			Npc_popSchedule(npc, -1);
			break;
		} else {
			if (Item_greatestDeltaToItem(*npc, first.current) < Item_greatestDeltaToItem(*npc, second.current))
				shutters = objref(first);
			else
				shutters = objref(second);
		}
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, shutters.type(), -1);
		break;
	case 1:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 2:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		facing = NPC(npc)->facing();
		/* the type the walk ended at is in result */
		FindNearestItem(&nearby, x, y, 3, 0, NPC(npc)->result, 0xff, 0xff);
		if (nearby.found()) {
			/* "These should be closed." and two more at open shutters, else "Ah, 'tis better!" and two more */
			if (NPC(npc)->result == 372 || NPC(npc)->result == 322)
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 113), 0, 15, 0);
			else
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 116), 0, 15, 0);
			RunUsable(1, nearby.current, -1);
		}
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 15, SCRIPT_END));
		if (CUR_SCHED(npc).x == -1)
			Npc_popSchedule(npc, nearby.found() ? 0 : -1);
		else {
			if (StartPath(*npc, CUR_SCHED(npc).x, CUR_SCHED(npc).y, 0, 100, DiscardedPathLength, 0) == 0)
				CUR_SCHED(npc).x = nearby.found();
			else
				Npc_popSchedule(npc, -1);
		}
		break;
	case 3:
		ContinueScheduleWalk(npc);
		break;
	case 4:
		Npc_popSchedule(npc, CUR_SCHED(npc).x ? 0 : -1);
		break;
	}
}
