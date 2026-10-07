/* Serpent Isle SI.EXE, overlay segment 258 (file offsets 0x06f660 to 0x06f8b8, 600 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "lowlevel.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "voolook.h"
#include "makemojo.h"
#include "combatai.h"
#include "missile.h"
#include "monsters.h"
#include "actitem.h"
#include "usehook.h"
#include "script.h"
#include "gtimer.h"
#include "sche.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Talk: walk up to the avatar and start a conversation once close and in sight. An NPC that cannot
 * reach him goes back to what its schedule says it does at this hour, or wanders if it has none.
 */
void RunTalkSchedule(objref *npc)
{
	int16_t monster;
	uint16_t type;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		type = objref(npc->off).type();
		monster = MonsterLookup.get(type);
		if (!PathNextToItem(npc, AvatarRef, 100) && (uint16_t)Item_getNpcNumber(npc) < 256) {
			uint8_t period, workType;

			period = GameTime.getHour() / 3;
			workType = Schedule_getWorkType(&ScheduleTable, Item_getNpcNumber(npc), period);
			if (workType == 0xff) {
				Npc_setSchedule(npc, WORK_WANDER);
				CUR_SCHED(npc).state = 1;
			}
			if (workType != 3 /* talk */) {
				CUR_SCHED(npc).state = 0;
				Npc_setSchedule(npc, workType);
				CUR_SCHED(npc).state = 1;
			}
		} else if (Item_greatestDeltaToItem(*npc, AvatarRef) <= 5 && HasLineOfFire(*npc, AvatarRef))
			CUR_SCHED(npc).state++;
		break;
	case 2:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
		break;
	case 3:
		RunUsable(9, *npc, -1);
		break;
	}
}
