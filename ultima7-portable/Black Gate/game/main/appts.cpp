/* Black Gate U7.EXE, overlay segment 210 (file offsets 0x0529a0 to 0x052e9c, 1276 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "gtimer.h"
#include "crime.h"
#include "sche.h"
#include "sche_ov1.h"
#include "coord.h"
#include "cheat.h"
#include "actitem.h"
#include "actutil.h"
#include "attack.h"
#include "mapview.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define IS_VALID(r) ((int8_t) ((r).off != 0))
#define NPC_FLAG(p, mask) ((uint8_t) ((NPC(p)->status & (mask)) != 0))
#define DEAD(p) ((uint8_t) ((int8_t)Item_getHitPoints(p) <= 0))
#define IS_KIND(p, expected) ((int8_t) (GetItemZAndStuff(p).kind() == (expected)))

extern objref AvatarRef;

int16_t unused_global_1 = -1;

void ResetNpcSchedules(void)
{
	int16_t npcNum;
	objref npc;
	uint8_t workType;
	uint8_t previous;

	for (npcNum = 0; npcNum < NPC_COUNT; npcNum++) {
		GetNpcIbo(&npc, npcNum);
		if (IS_VALID(npc)) {
			previous = NPC(&npc)->workType;
			if (npcNum < 256 && NPC(&npc)->workType != WORK_WAIT) {
				workType = Schedule_getWorkType(&ScheduleTable, npcNum, GameTime.getHour() / 3);
				if (workType == 255) {
					Npc_setSchedule(&npc, previous);
					CUR_SCHED(&npc).state = 1;
				} else
					Npc_setSchedule(&npc, workType);
			} else {
				Npc_setSchedule(&npc, previous);
				CUR_SCHED(&npc).state = 1;
			}
			NPC(&npc)->food = 18;
		}
	}
	KillNpcMode = 0;
}

void UpdateNpcSchedules(void)
{
	objref npc;
	uint8_t workType;
	Coord x, y;
	uint8_t busy;
	int16_t npcNum;

	SchedulePeriod = GameTime.getHour() / 3;
	for (npcNum = 0; npcNum < 256; npcNum++) {
		if (DoScheduleNpc != -1 && DoScheduleNpc != npcNum)
			continue;
		GetNpcIbo(&npc, npcNum);
		/* skip party members, the paralyzed, the dead and the asleep */
		if (IS_VALID(npc) && !NPC_FLAG(&npc, NPC_IN_PARTY)
			&& !NPC_FLAG(&npc, NPC_PARALYZED) && !DEAD(&npc)
			&& !NPC_FLAG(&npc, NPC_DEAD)
			&& (!NPC_FLAG(&npc, NPC_ASLEEP) || NPC(&npc)->workType == WORK_SLEEP)
			&& NPC(&npc)->workType != WORK_WAIT) {
			busy = IS_KIND(&npc, 4) ? 0 : (int16_t) IsItemInCellWindow(npc);
			if (NPC(&npc)->workType != WORK_COMBAT || !busy) {
				workType = Schedule_getWorkType(&ScheduleTable,
					Item_getNpcNumber(&npc), SchedulePeriod);
				if (workType == 255)
					continue;
				Schedule_getCoord(&ScheduleTable, Item_getNpcNumber(&npc),
					SchedulePeriod, &x, &y);
				if (NPC(&npc)->workType == workType &&
					(GetDistance(Item_getX(npc), Item_getY(npc),
					GetItemZAndStuff(&npc).z(), x, y, 0) <= 25
					|| NPC(&npc)->workType == WORK_PATROL))
					continue;
				if (NPC(&npc)->workType == WORK_SLEEP)
					WakeUpNpc(&npc);
				Npc_setSchedule(&npc, workType);
				if (!busy) {
					if (GetDistance(Item_getX(AvatarRef),
						Item_getY(AvatarRef), GetItemZAndStuff(&AvatarRef).z(), x, y, 0) >= 24)
						Item_move(&npc, CellCoord(x), CellCoord(y));
					else if (!PlaceNpcNearAvatar(&npc, x, y, 1)) {
						Npc_setSchedule(&npc, WORK_WANDER);
						CUR_SCHED(&npc).state = 1;
					}
				}
			}
		}
	}
}
