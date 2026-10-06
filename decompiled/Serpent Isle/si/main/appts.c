/* Serpent Isle SI.EXE, overlay segment 344 (file offsets 0x0a1ff0 to 0x0a23f7, 1031 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

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
#define IS_VALID(r) ((char) ((r).off != 0))
#define NPC_FLAG(p, mask) ((unsigned char) ((NPC(p)->status & (mask)) != 0))
#define DEAD(p) ((unsigned char) ((char)Item_getHitPoints(p) <= 0))
#define IS_KIND(p, expected) ((char) (GetItemZAndStuff(p).kind() == (expected)))

extern objref AvatarRef;

void far UpdateNpcSchedules(void)
{
	objref npc;
	unsigned char workType;
	Coord x, y;
	unsigned char busy;
	int npcNum;

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
			&& NPC(&npc)->workType != WORK_WAIT
			&& NPC(&npc)->workType != 24) {
			busy = IS_KIND(&npc, 4) ? 0 : (int) IsItemInCellWindow(npc);
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
						Item_getY(AvatarRef), GetItemZAndStuff(&AvatarRef).z(), x, y, 0) >= 24) {
						Item_move(&npc, x, y, 0);
					} else if (!PlaceNpcNearAvatar(&npc, x, y, 1)) {
						Npc_setSchedule(&npc, WORK_WANDER);
						CUR_SCHED(&npc).state = 1;
					}
				}
			}
		}
	}
}
