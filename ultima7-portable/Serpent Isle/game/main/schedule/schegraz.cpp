/* Serpent Isle SI.EXE, overlay segment 282 (file offsets 0x07af50 to 0x07b2bd, 877 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "npcpath.h"
#include "actitem.h"
#include "actqueue.h"
#include "script.h"

extern int16_t DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Graze: wander within seven cells of where the activity started; fish leap now and then. */
void RunGrazeSchedule(objref *npc)
{
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
		if (NPC(npc)->scheduleValue > 2)
			CUR_SCHED(npc).state = 4;
		else if (StartPath(*npc,
			GenerateRandomIntegerInRange(15) + Npc_getScheduleVariable0(npc) - 7,
			GenerateRandomIntegerInRange(15) + Npc_getScheduleVariable1(npc) - 7,
			Item_getZ(npc), 25, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else
			NPC(npc)->scheduleValue++;
		break;
	case 3:
		if (npc->type() == 509 /* fish */) {
			if (RollChance(15))
				ActionQueue.add(*npc, MakeScript(SCRIPT_UP_FRAME, SCRIPT_OUT_FRAME, SCRIPT_END));
			else if (RollChance(3))
				ContinueScheduleWalk(npc);
		} else
			ContinueScheduleWalk(npc);
		break;
	case 4:
		NPC(npc)->scheduleValue = 0;
		if (npc->type() == 509 /* fish */)
			CUR_SCHED(npc).state = 2;
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, GenerateRandomIntegerInRange(4) + 2,
				SCRIPT_UP_FRAME, SCRIPT_WAIT, GenerateRandomIntegerInRange(4) + 2, SCRIPT_OUT_FRAME, SCRIPT_WAIT,
				GenerateRandomIntegerInRange(4) + 2, SCRIPT_LOOP, -6, GenerateRandomIntegerInRange(4) + 2,
				SCRIPT_END));
		break;
	case 5:
		CUR_SCHED(npc).state = 2;
		break;
	}
}
