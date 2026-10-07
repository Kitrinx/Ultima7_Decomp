/* Serpent Isle SI.EXE, overlay segment 285 (file offsets 0x07b870 to 0x07beee, 1662 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
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
 * Hound someone: a rat hounds NPC 27, a wolf would hound a wolf among NPCs 256-355, anyone else the
 * avatar. Keep within five cells of them, now and then running the NPC's usecode or pacing nearby.
 * An NPC that cannot see an invisible quarry wanders within ten cells instead.
 */
void DoWorkHound(objref *npc)
{
	int16_t monster;
	NPCRef quarry;
	int16_t i;

	switch (npc->type()) {
	case 523 /* rat */:
		GetNpcIbo(&quarry, 27);
		break;
	case 537 /* wolf */:
		for (i = 256; i < 356; i++) {
			GetNpcIbo(&quarry, i);
			if (quarry.type() == 537 && Npc_getMagic(&quarry) == 9)
				break;
		}
		/* assigns rather than compares, so a wolf always gives up here */
		if (i = 356)
			return;
		break;
	default:
		GetNpcIbo(&quarry, Item_getNpcNumber(&AvatarRef));
		break;
	}

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		CUR_SCHED(npc).counter = 0;
		i = objref(npc->off).type();
		monster = MonsterLookup.get(i);
		/* quality flag bit 0: the quarry is invisible */
		if (FLAG(Item_getQualityFlags(&quarry), QUALITY_INVISIBLE)
			&& !(uint8_t) MonsterRecords.get(monster)->seeInvisible) {
			if (StartPath(*npc,
				Coord(GenerateRandomIntegerInRange(21) + Item_getX(*npc).value - 10),
				Coord(GenerateRandomIntegerInRange(21) + Item_getY(*npc).value - 10),
				Item_getZ(npc), 30, DiscardedPathLength, 0) == 0) {
				CUR_SCHED(npc).state++;
				CUR_SCHED(npc).counter = 1;
			} else if (RollChance(4))
				CUR_SCHED(npc).state = 3;
		} else if (Item_greatestDeltaToItem(*npc, quarry) > 5) {
			if (!PathNextToItem(npc, quarry, 60)) {
				CUR_SCHED(npc).counter = 1;
				CUR_SCHED(npc).state = 3;
			}
		} else if (RollChance(10) && IsOnOutermostSchedule(npc))
			RunUsable(0, *npc, -1);
		else if (RollChance(100) && !IsOnOutermostSchedule(npc))
			Npc_popSchedule(npc, 0);
		else if (RollChance(10)) {
			if (StartPath(*npc,
				Coord(GenerateRandomIntegerInRange(11) + Item_getX(quarry).value - 5),
				Coord(GenerateRandomIntegerInRange(11) + Item_getY(quarry).value - 5),
				Item_getZ(&quarry), 30, DiscardedPathLength, 0) == 0)
				CUR_SCHED(npc).state++;
		}
		break;
	case 2:
		ContinueScheduleWalk(npc);
		if (!CUR_SCHED(npc).counter && Item_greatestDeltaToItem(*npc, quarry) > 5) {
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
