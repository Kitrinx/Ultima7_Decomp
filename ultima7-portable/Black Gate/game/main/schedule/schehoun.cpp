/* Black Gate U7.EXE, overlay segment 301 (file offsets 0x08c6d0 to 0x08ccd7, 1543 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "voolook.h"
#include "makemojo.h"
#include "monsters.h"
#include "random.h"
#include "npcpath.h"
#include "actitem.h"
#include "combatai.h"
#include "usehook.h"
#include "script.h"

extern int16_t DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FLAG(v, m) ((uint8_t) ((v) & (m)))

/*
 * Hound the avatar: keep within five cells of him, now and then running the NPC's usecode or pacing
 * nearby. An NPC that cannot see the invisible avatar wanders within ten cells instead.
 */
void DoWorkHound(objref *npc)
{
	int16_t monster;
	uint16_t type;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		CUR_SCHED(npc).counter = 0;
		type = objref(npc->off).type();
		monster = MonsterLookup.get(type);
		/* quality flag bit 0: the avatar is invisible */
		if (FLAG(Item_getQualityFlags(&AvatarRef), QUALITY_INVISIBLE)
			&& !(uint8_t) MonsterRecords.get(monster)->seeInvisible) {
			if (StartPath(*npc,
				Coord(GenerateRandomIntegerInRange(21) + Item_getX(*npc).value - 10),
				Coord(GenerateRandomIntegerInRange(21) + Item_getY(*npc).value - 10),
				Item_getZ(npc), 30, DiscardedPathLength, 0) == 0) {
				CUR_SCHED(npc).state++;
				CUR_SCHED(npc).counter = 1;
			} else if (RollChance(4))
				CUR_SCHED(npc).state = 3;
		} else if (Item_greatestDeltaToItem(*npc, AvatarRef) > 5) {
			if (!PathNextToItem(npc, AvatarRef, 60)) {
				CUR_SCHED(npc).counter = 1;
				CUR_SCHED(npc).state = 3;
			}
		} else if (RollChance(10) && IsOnOutermostSchedule(npc))
			RunUsable(0, *npc, -1);
		else if (RollChance(100) && !IsOnOutermostSchedule(npc))
			Npc_popSchedule(npc, 0);
		else if (RollChance(10)) {
			if (StartPath(*npc,
				Coord(GenerateRandomIntegerInRange(11) + Item_getX(AvatarRef).value - 5),
				Coord(GenerateRandomIntegerInRange(11) + Item_getY(AvatarRef).value - 5),
				Item_getZ(&AvatarRef), 30, DiscardedPathLength, 0) == 0)
				CUR_SCHED(npc).state++;
		}
		break;
	case 2:
		ContinueScheduleWalk(npc);
		if (!CUR_SCHED(npc).counter && Item_greatestDeltaToItem(*npc, AvatarRef) > 5) {
			StopPaths(*npc);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 3:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, CUR_SCHED(npc).counter ? GenerateRandomIntegerInRange(11) + 10
			: GenerateRandomIntegerInRange(5) + 3, SCRIPT_END));
		break;
	case 4:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
