/* Black Gate U7.EXE, overlay segment 284 (file offsets 0x085260 to 0x08561e, 958 bytes).
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
#include "search.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Lab work: stir at the cauldron and now and then carry a potion to a table. */
void RunLabSchedule(objref *npc)
{
	AreaSearch cauldron;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_READ, -1, -1);
		break;
	case 2:
		if (RollChance(3))
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		else
			CUR_SCHED(npc).state++;
		break;
	case 3:
		Npc_pushSchedule(npc, WORK_GRAB_ITEM, MakeTypeFrame(340 /* potion */, RollRandom(8)), 1);
		break;
	case 4:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, 995 /* cauldron */, -1);
		break;
	case 5:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else {
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
			if (RollChance(2)) {
				FindNearestItem(&cauldron, Item_getX(*npc), Item_getY(*npc), 1, 0, 995 /* cauldron */, 0xff, 0xff);
				if (cauldron.found())
					PostScriptToItem(&cauldron.current, MakeScript(SCRIPT_WAIT, 6,
						SCRIPT_FRAME, GenerateRandomIntegerInRange(4) + 1,
						SCRIPT_WAIT, GenerateRandomIntegerInRange(91) + 40, SCRIPT_FRAME, 0, SCRIPT_END));
			}
		}
		break;
	case 6:
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		break;
	case 7:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 340 /* potion */, RollRandom(2) ? 1018 /* table */ : 1003 /* table */);
		break;
	case 8:
		if (RollChance(4))
			CUR_SCHED(npc).state = 1;
		else
			CUR_SCHED(npc).state = 2;
		break;
	}
}
