/* Black Gate U7.EXE, overlay segment 294 (file offsets 0x08a240 to 0x08b4fc, 4796 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "search.h"
#include "npcpath.h"
#include "actitem.h"
#include "equip.h"
#include "combat.h"
#include "usehook.h"
#include "wihh.h"
#include "script.h"
#include "iteminfo.h"
#include "mapview.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

extern int DiscardedPathLength[2];

/* frames of the plant a patroller lays on a grave */
char GraveFlowerFrames[5] = { 1, 2, 3, 6, 7 };

extern objref AvatarRef;

/* Wake every patroller waiting at a stop (state 30). */
void far ResumeWaitingPatrols()
{
	objref npc;

	for (int number = 0; number < NPC_COUNT; number++) {
		GetNpcIbo(&npc, number);
		if (npc.valid() && CanVisit(&npc) && NPC(&npc)->workType == WORK_PATROL &&
			CUR_SCHED(&npc).state == 30)
			CUR_SCHED(&npc).state++;
	}
}

/*
 * Patrol: walk from path marker to path marker, numbered by frame, back and forth along the route.
 * Each marker's quality says what to do on arrival (low five bits) and whether to attack an avatar
 * nearby (0x20).
 */
void far RunPatrolSchedule(objref *npc)
{
	unsigned char result;
	int flower;
	objref held;
	AreaSearch marker;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		FindNearestItem(&marker, Item_getX(*npc), Item_getY(*npc),
			25, 0x80, 607 /* path */, 0xff, 0xff);
		if (marker.current.valid())
			++CUR_SCHED(npc).state;
		else
			CUR_SCHED(npc).state = 40;
		break;
	case 2:
		CUR_SCHED(npc).x = 0;
		CUR_SCHED(npc).y = 0;
		CUR_SCHED(npc).counter = 0;
		++CUR_SCHED(npc).state;
		break;
	case 3:
		/* 0x40: act at this marker again instead of walking on */
		if ((unsigned char) (CUR_SCHED(npc).counter & 0x40)) {
			NPC(npc)->typeFlagsHigh = NPC(npc)->typeFlagsHigh & ~8;
			NPC(npc)->result = 0;
			CUR_SCHED(npc).state = 5;
		} else {
			/* the second argument says which way along the route */
			if (CUR_SCHED(npc).y) {
				if ((unsigned) CUR_SCHED(npc).x < 31)
					++CUR_SCHED(npc).x;
				else
					CUR_SCHED(npc).y = 0;
			} else {
				if ((unsigned) CUR_SCHED(npc).x > 0)
					--CUR_SCHED(npc).x;
				else
					CUR_SCHED(npc).y = 1;
			}
			FindNearestItem(&marker, Item_getX(*npc), Item_getY(*npc),
				25, 0x80, 607 /* path */, 0xff, CUR_SCHED(npc).x);
			if (marker.current.valid()) {
				CUR_SCHED(npc).counter = (unsigned) (unsigned char) Item_getQuality(&marker.current);
				result = StartPath(*npc, Item_getX(marker.current), Item_getY(marker.current),
					Item_getZ(&marker.current), 100, DiscardedPathLength, 0);
				if (result == 0)
					++CUR_SCHED(npc).state;
				else if (result == 2)
					CUR_SCHED(npc).y = !CUR_SCHED(npc).y;
			} else
				CUR_SCHED(npc).y = !CUR_SCHED(npc).y;
		}
		break;
	case 4:
		ContinueScheduleWalk(npc);
		break;
	case 5:
		if (IsBlocked(npc))
			CUR_SCHED(npc).y = !CUR_SCHED(npc).y;
		else {
			switch (CUR_SCHED(npc).counter & 0x1f) {
			case 1:
				CUR_SCHED(npc).x = 0;
				CUR_SCHED(npc).y = 0;
				break;
			case 2:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_PAUSE, 15, -1);
				return;
			case 3:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_PAUSE, GenerateRandomIntegerInRange(46) + 45, -1);
				CUR_SCHED(npc).state = 255;
				Npc_pushSchedule(npc, WORK_SIT, -1, -1);
				return;
			case 4:
				CUR_SCHED(npc).state = 2;
				flower = GenerateRandomIntegerInRange(5);
				Npc_pushSchedule(npc, WORK_PAY_RESPECTS, 715 /* tombstone */,
					MakeTypeFrame(999 /* plant */, GraveFlowerFrames[flower]));
				return;
			case 5:
				PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_KNEEL_FRAME, SCRIPT_WAIT, 20, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
					SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
				break;
			case 6:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_LOITER, -1, -1);
				CUR_SCHED(npc).state = 1;
				return;
			case 7:
				PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_FACE,
					(GetFacing(npc) + 54) & 7, SCRIPT_WAIT, 2, SCRIPT_FACE,
					(GetFacing(npc) + 52) & 7, SCRIPT_WAIT, 2, SCRIPT_END));
				break;
			case 8:
				PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_FACE,
					(GetFacing(npc) + 50) & 7, SCRIPT_WAIT, 2, SCRIPT_FACE,
					(GetFacing(npc) + 52) & 7, SCRIPT_WAIT, 2, SCRIPT_END));
				break;
			case 9:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_HOR_PACE, -1, -1);
				CUR_SCHED(npc).state = 1;
				return;
			case 10:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_VER_PACE, -1, -1);
				CUR_SCHED(npc).state = 1;
				return;
			case 11:
				if (RollChance(2))
					CUR_SCHED(npc).y = !CUR_SCHED(npc).y;
				break;
			case 12:
				if (RollChance(2)) {
					if (CUR_SCHED(npc).y) {
						if ((unsigned) CUR_SCHED(npc).x < 31)
							++CUR_SCHED(npc).x;
						else
							CUR_SCHED(npc).y = 0;
					} else {
						if ((unsigned) CUR_SCHED(npc).x > 0)
							--CUR_SCHED(npc).x;
						else
							CUR_SCHED(npc).y = 1;
					}
				}
				break;
			case 13:
				CUR_SCHED(npc).state = 20;
				return;
			case 14:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
				return;
			case 15:
				RunUsable(0, npc->off, -1);
				break;
			case 16:
				PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
				break;
			case 17:
				PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME,
					SCRIPT_WAIT, 2, SCRIPT_END));
				break;
			case 18:
				CUR_SCHED(npc).state = 30;
				return;
			case 19:
				ResumeWaitingPatrols();
				break;
			case 20:
				SelectWeapon(*npc, 0, 0);
				break;
			case 21:
				held = objref(GetItemInSlot(*npc, 1));
				if (held.valid())
					EquipItem(held, *npc, 0, 0);
				break;
			case 22:
				PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_EXTEND1_FRAME,
					SCRIPT_THRUST1_FRAME, SCRIPT_READY_FRAME, SCRIPT_STAND_FRAME, SCRIPT_END));
				break;
			case 23:
				PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_RAISE2_FRAME, SCRIPT_EXTEND2_FRAME,
					SCRIPT_THRUST2_FRAME, SCRIPT_READY_FRAME, SCRIPT_STAND_FRAME, SCRIPT_END));
				break;
			case 24:
				CUR_SCHED(npc).state = 2;
				Npc_pushSchedule(npc, WORK_READ, -1, -1);
				return;
			case 25:
				if (RollChance(2)) {
					CUR_SCHED(npc).x = 0;
					CUR_SCHED(npc).y = 0;
				}
				break;
			}
			/* 0x20: turn on an avatar within eight cells */
			if ((unsigned char) (CUR_SCHED(npc).counter & 0x20)) {
				if (Item_greatestDeltaToItem(*npc, AvatarRef) <= 8) {
					Npc_setSchedule(npc, WORK_COMBAT);
					CUR_SCHED(npc).state = 1;
				}
			}
		}
		CUR_SCHED(npc).state = 3;
		break;
	case 20:
		Npc_pushSchedule(npc, WORK_READY_HAND, 623 /* hammer */, 1);
		break;
	case 21:
		PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_RAISE1_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_OUT_FRAME, SCRIPT_SFX, 45, SCRIPT_WAIT, 2, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2, SCRIPT_LOOP, 0,
			GenerateRandomIntegerInRange(3) + 1, SCRIPT_END));
		break;
	case 22:
		CUR_SCHED(npc).state = 3;
		break;
	case 31:
		CUR_SCHED(npc).state = 3;
		break;
	case 40:
		if (NPC(npc)->result == 1) {
			Npc_setSchedule(npc, WORK_COMBAT);
			CUR_SCHED(npc).state = 1;
		} else {
			Npc_pushSchedule(npc, WORK_LOITER, CUR_SCHED(npc).state, -1);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 41:
		CUR_SCHED(npc).state = 40;
		break;
	}
}
