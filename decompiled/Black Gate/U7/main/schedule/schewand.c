/* Black Gate U7.EXE, overlay segment 280 (file offsets 0x083090 to 0x083360, 720 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "npcref.h"
#include "combatai.h"
#include "npcpath.h"
#include "actitem.h"
#include "script.h"

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Wander to random spots within fifteen cells; after three failed walks, pause a while. */
void far RunWanderSchedule(NPCRef *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 2:
		if (RollChance(3) && IsSentient(*npc)) {
			NPC(npc)->scheduleValue = 0;
			CUR_SCHED(npc).state = 1;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, -1, -1);
		} else if (NPC(npc)->scheduleValue > 2)
			CUR_SCHED(npc).state = 4;
		else if (StartPath(*npc,
			Coord(GenerateRandomIntegerInRange(31) + Item_getX(*npc).value - 15),
			Coord(GenerateRandomIntegerInRange(31) + Item_getY(*npc).value - 15),
			Item_getZ(npc), 45, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else
			NPC(npc)->scheduleValue++;
		break;
	case 3:
		ContinueScheduleWalk(npc);
		break;
	case 4:
		/* fliers never stop to rest */
		if ((unsigned char) (NPC(npc)->typeFlags & 0x10))
			CUR_SCHED(npc).state = 1;
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(21) + 20, SCRIPT_END));
		break;
	case 5:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
