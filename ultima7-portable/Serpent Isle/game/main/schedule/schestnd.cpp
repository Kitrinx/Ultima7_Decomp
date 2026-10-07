/* Serpent Isle SI.EXE, overlay segment 284 (file offsets 0x07b7f0 to 0x07b862, 114 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "actitem.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Stand: go to the scheduled spot and stand there. */
void RunStandSchedule(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
		break;
	}
}
