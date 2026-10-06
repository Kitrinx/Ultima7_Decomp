/* Serpent Isle SI.EXE, overlay segment 263 (file offsets 0x070900 to 0x070f15, 1557 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "iteminfo.h"
#include "coord.h"
#include "u7npc.h"
#include "item.h"
#include "combatai.h"
#include "random.h"
#include "npcref.h"
#include "usehook.h"
#include "npcpath.h"
#include "actitem.h"
#include "script.h"

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FLAG(v, m) ((unsigned char) ((v) & (m)))

inline int RandomInRange(int lo, int hi)
{
	return lo <= hi ? GenerateRandomIntegerInRange(hi - lo + 1) + lo : hi;
}

/*
 * Loiter: walk to random spots around where the activity started, pausing between walks. Pushed over
 * another schedule, it returns to that one now and then.
 */
void far RunLoiterSchedule(NPCRef *npc)
{
	int ticks;
	int workType;
	int range;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		NPC(npc)->sVr[0] = Loc(Item_getX(*npc)).value;
		NPC(npc)->sVr[1] = Loc(Item_getY(*npc)).value;
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 2:
		if (CanMove(npc))
			CUR_SCHED(npc).state++;
		else
			CUR_SCHED(npc).state = 5;
		break;
	case 3:
		if (IsOnOutermostSchedule(npc) && RollChance(3) && IsSentient(*npc)) {
			NPC(npc)->scheduleValue = 0;
			CUR_SCHED(npc).state = 2;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1, 0);
		} else if (IsOnOutermostSchedule(npc) && RollChance(15))
			RunUsable(0, *npc, -1);
		else if (NPC(npc)->scheduleValue > 2)
			CUR_SCHED(npc).state = 5;
		else {
			workType = NPC(npc)->workType;
			if (workType == WORK_COMBAT || workType == WORK_PATROL)
				range = 8;
			else
				range = 15;
			if (StartPath(*npc, Npc_getScheduleVariable0(npc) + RandomInRange(-range, range),
				Npc_getScheduleVariable1(npc) + RandomInRange(-range, range), Item_getZ(npc),
				workType == WORK_COMBAT || workType == WORK_PATROL ? 15 : 30, DiscardedPathLength, 0) == 0)
				CUR_SCHED(npc).state++;
			else
				NPC(npc)->scheduleValue++;
		}
		break;
	case 4:
		ContinueScheduleWalk(npc);
		break;
	case 5:
		NPC(npc)->scheduleValue = 0;
		/* fliers never stop to rest */
		if (!FLAG(NPC(npc)->typeFlags, 0x10)) {
			ticks = IsOnOutermostSchedule(npc) ? GenerateRandomIntegerInRange(21) + 20 :
				GenerateRandomIntegerInRange(11) + 15;
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, ticks, SCRIPT_END));
		} else
			CUR_SCHED(npc).state++;
		break;
	case 6:
		if (IsOnOutermostSchedule(npc))
			CUR_SCHED(npc).state = 2;
		else {
			/* a patrol with no path pushes this loiter from its state 40 */
			if (NPC(npc)->workType == WORK_PATROL && CUR_SCHED(npc).x == 40 &&
				Item_greatestDeltaToItem(*npc, AvatarRef) <= 10)
				Npc_popSchedule(npc, 1);
			else if (NPC(npc)->workType == WORK_COMBAT || RollChance(4)) {
				if (CanMove(npc))
					Npc_pushSchedule(npc, WORK_GOTO_REM_L, -1, -1, 0);
				else
					Npc_popSchedule(npc, 0);
			} else
				CUR_SCHED(npc).state = 2;
		}
		break;
	case 7:
		Npc_popSchedule(npc, IsBlocked(npc) ? -1 : 0);
		break;
	}
}
