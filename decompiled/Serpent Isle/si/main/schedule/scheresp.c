/* Serpent Isle SI.EXE, overlay segment 289 (file offsets 0x07d290 to 0x07d52e, 670 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "actitem.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Pay respects: walk to an item of the type the first argument names and kneel there, laying down an
 * offering of the type the second names unless it is -1.
 */
void far DoWorkPayRespects(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		if (CUR_SCHED(npc).y == -1)
			CUR_SCHED(npc).state++;
		else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, CUR_SCHED(npc).y, 1, 0);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, CUR_SCHED(npc).x, -1, 0);
		break;
	case 2:
		if (CUR_SCHED(npc).y == -1)
			CUR_SCHED(npc).state++;
		else
			Npc_pushSchedule(npc, WORK_DROP_ITEM, CUR_SCHED(npc).y, -1, 0);
		break;
	case 3:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_KNEEL_FRAME, SCRIPT_WAIT, 3, SCRIPT_WAIT, 40, SCRIPT_END));
		break;
	case 4:
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_UP_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
		break;
	case 5:
		Npc_popSchedule(npc, 0);
		break;
	}
}
