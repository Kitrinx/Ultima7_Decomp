/* Serpent Isle SI.EXE, overlay segment 292 (file offsets 0x07e490 to 0x07e6bd, 557 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "random.h"
#include "actitem.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Fencing practice: take up a two-handed sword and swing at the dummy. */
void DoWorkFencing(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_READY_HAND, 602 /* two-handed sword */, 1, 0);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, 860 /* fencing dummy */, -1, 0);
		break;
	case 2:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			CUR_SCHED(npc).state++;
		break;
	case 3:
		if (RollChance(2))
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE1_FRAME, SCRIPT_WAIT, 1,
				SCRIPT_SFX, 9, SCRIPT_EXTEND1_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST1_FRAME, SCRIPT_SFX, 125,
				SCRIPT_WAIT, 2, SCRIPT_READY_FRAME, SCRIPT_WAIT, 4, SCRIPT_END));
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE2_FRAME, SCRIPT_WAIT, 1,
				SCRIPT_SFX, 9, SCRIPT_EXTEND2_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST2_FRAME, SCRIPT_SFX, 125,
				SCRIPT_WAIT, 4, SCRIPT_READY_FRAME, SCRIPT_WAIT, 4, SCRIPT_END));
		break;
	case 4:
		if (RollChance(3))
			Npc_popSchedule(npc, 0);
		else
			CUR_SCHED(npc).state = 3;
		break;
	}
}
