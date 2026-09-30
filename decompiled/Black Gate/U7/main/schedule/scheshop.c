/* Black Gate U7.EXE, overlay segment 299 (file offsets 0x08c160 to 0x08c5eb, 1163 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "coord.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "random.h"
#include "combatai.h"
#include "npcpath.h"
#include "actitem.h"
#include "usehook.h"
#include "script.h"

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Tend shop: loiter within eight cells of where the activity started, and now and then go to meet an
 * avatar who comes within twenty cells of that spot.
 */
void far RunTendShopSchedule(NPCRef *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		NPC(npc)->sVr[0] = Loc(Item_getX(*npc)).value;
		NPC(npc)->sVr[1] = Loc(Item_getY(*npc)).value;
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 2:
		if (RollChance(3) && IsSentient(*npc)) {
			NPC(npc)->scheduleValue = 0;
			CUR_SCHED(npc).state = 1;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		} else if (NPC(npc)->scheduleValue > 3)
			CUR_SCHED(npc).state = 4;
		else if (RollChance(2)) {
			NPC(npc)->scheduleValue = 0;
			CUR_SCHED(npc).state = 10;
		} else if (StartPath(*npc,
			GenerateRandomIntegerInRange(17) + NPC(npc)->sVr[0] - 8,
			GenerateRandomIntegerInRange(17) + NPC(npc)->sVr[1] - 8,
			0, 15, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else
			NPC(npc)->scheduleValue++;
		break;
	case 3:
		ContinueScheduleWalk(npc);
		break;
	case 4:
		NPC(npc)->scheduleValue = 0;
		if (RollChance(5))
			RunUsable(0, *npc, -1);
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(26) + 25, SCRIPT_END));
		break;
	case 5:
		CUR_SCHED(npc).state = 2;
		break;
	case 10:
		if (Item_greatestDeltaToItem(*npc, AvatarRef) <= 2) {
			if (RollChance(2))
				RunUsable(0, *npc, -1);
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, GenerateRandomIntegerInRange(11) + 15,
				SCRIPT_END));
		} else if (Item_greatestDeltaToCoords(AvatarRef, NPC(npc)->sVr[0],
			NPC(npc)->sVr[1], 0) < 20) {
			if (!PathNextToItem(npc, AvatarRef, 50))
				CUR_SCHED(npc).state = 2;
		} else
			CUR_SCHED(npc).state = 2;
		break;
	case 11:
		CUR_SCHED(npc).state = 2;
		break;
	}
}
