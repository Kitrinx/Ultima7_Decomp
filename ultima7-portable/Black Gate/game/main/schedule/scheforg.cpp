/* Black Gate U7.EXE, overlay segment 281 (file offsets 0x0833a0 to 0x083c87, 2279 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "actitem.h"
#include "actmove.h"
#include "search.h"
#include "actutil.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)

/* Blacksmith: heat a sword blank in the firepit, hammer it on the anvil and quench it in the trough. */
void RunBlacksmithSchedule(objref *npc)
{
	objref blank;
	AreaSearch nearby;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		if (RollChance(3)) {
			CUR_SCHED(npc).state = 0;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		} else
			Npc_pushSchedule(npc, WORK_MOVE_ITEM, MakeTypeFrame(668 /* sword blank */, 0), 739 /* firepit */);
		break;
	case 2:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 431 /* bellows */, -1);
		break;
	case 3:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else {
			/* work the bellows while the firepit and the blank heat up */
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_LOOP, 0, 7, SCRIPT_END));
			FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
				5, 0, 431 /* bellows */, 0xff, 0xff);
			PostScriptToItem(&nearby.current, MakeScript(SCRIPT_FRAME, 0, SCRIPT_WAIT, 2, SCRIPT_FRAME, 1,
				SCRIPT_WAIT, 2, SCRIPT_SFX, 47, SCRIPT_CONTINUE, SCRIPT_FRAME, 2, SCRIPT_WAIT, 2, SCRIPT_FRAME, 1,
				SCRIPT_WAIT, 2, SCRIPT_FRAME, 0, SCRIPT_WAIT, 2, SCRIPT_LOOP, 0, 7, SCRIPT_END));
			FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
				10, 0, 739 /* firepit */, 0xff, 0xff);
			PostScriptToItem(&nearby.current, MakeScript(SCRIPT_FINISH, SCRIPT_WAIT, 34, SCRIPT_NEXT_FRAME_MAX,
				SCRIPT_WAIT, 29, SCRIPT_NEXT_FRAME_MAX, SCRIPT_WAIT, 29, SCRIPT_NEXT_FRAME_MAX, SCRIPT_WAIT, 45,
				SCRIPT_PREV_FRAME, SCRIPT_WAIT, 60, SCRIPT_PREV_FRAME, SCRIPT_WAIT, 105, SCRIPT_PREV_FRAME,
				SCRIPT_END));
			FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
				10, 0, 739 /* firepit */, 0xff, 0xff);
			if (nearby.found())
				FindNearestItem(&nearby, Item_getX(nearby.current), Item_getY(nearby.current),
					10, 0, 668 /* sword blank */, 0xff, 0xff);
			else
				FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
					10, 0, 668 /* sword blank */, 0xff, 0xff);
			PostScriptToItem(&nearby.current, MakeScript(SCRIPT_FINISH, SCRIPT_WAIT, 34, SCRIPT_NEXT_FRAME_MAX,
				SCRIPT_WAIT, 14, SCRIPT_NEXT_FRAME_MAX, SCRIPT_LOOP, -3, 3, SCRIPT_END));
		}
		break;
	case 4:
		Npc_pushSchedule(npc, WORK_READY_HAND, 994 /* tongs */, 1);
		break;
	case 5:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 668 /* sword blank */, 991 /* anvil */);
		break;
	case 6:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_READY_HAND, 623 /* hammer */, 1);
		break;
	case 7:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 991 /* anvil */, -1);
		break;
	case 8:
		PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_RAISE1_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_OUT_FRAME, SCRIPT_SFX, 45, SCRIPT_WAIT, 2, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_LOOP, 0, 2,
			SCRIPT_END));
		FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
			10, 0, 668 /* sword blank */, 0xff, 0xff);
		PostScriptToItem(&nearby.current, MakeScript(SCRIPT_WAIT, 8, SCRIPT_PREV_FRAME_MIN, SCRIPT_LOOP, 0, 2,
			SCRIPT_END));
		break;
	case 9:
		StowHeldItems(npc);
		FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
			25, 0, 719 /* water trough */, 0xff, 0xff);
		if (nearby.found() && FRAME(ITEM(nearby.current.off)) == 0)
			Npc_pushSchedule(npc, WORK_FILL_WATER_TROUGH, -1, -1);
		else
			CUR_SCHED(npc).state++;
		break;
	case 10:
		Npc_pushSchedule(npc, WORK_GRAB_ITEM, 668 /* sword blank */, -1);
		break;
	case 11:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, 719 /* water trough */, -1);
		break;
	case 12:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else {
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_LOOP, 0, 2, SCRIPT_END));
			FindNearestItem(&nearby, Item_getX(*npc), Item_getY(*npc),
				5, 0, 719 /* water trough */, 0xff, 0xff);
			PostScriptToItem(&nearby.current, MakeScript(SCRIPT_WAIT, 7, SCRIPT_SFX, 46,
				SCRIPT_WAIT, 15, SCRIPT_SFX, 46, SCRIPT_WAIT, 15, SCRIPT_SFX, 46, SCRIPT_CONTINUE,
				SCRIPT_PREV_FRAME_MIN, SCRIPT_END));
		}
		break;
	case 13:
		blank = FindCarriedItem(npc, 668 /* sword blank */);
		Item_setFrame(&blank, 0);
		CUR_SCHED(npc).state++;
		break;
	case 14:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 623 /* hammer */, 1003 /* table */);
		break;
	case 15:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 994 /* tongs */, 1003 /* table */);
		break;
	case 16:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
