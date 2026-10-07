/* Serpent Isle SI.EXE, overlay segment 297 (file offsets 0x0804b0 to 0x080819, 873 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "actmove.h"
#include "random.h"
#include "combat.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define IN_PARTY(n) ((uint8_t) (((n)->status & NPC_IN_PARTY) != 0))

/* the check picked in state 1 */
inline int16_t GetChoice(objref *p)
{
	return CUR_SCHED(p).y;
}

/*
 * Look around: check the lights or the shutters, whichever comes up, then the other if that failed.
 * An NPC outside the party that put its weapon away takes it up again afterwards.
 */
void DoWorkCheckArea(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		if (StowHeldItems(npc) && !IN_PARTY(NPC(npc)))
			CUR_SCHED(npc).counter = 1;
		else
			CUR_SCHED(npc).counter = 0;
		CUR_SCHED(npc).state++;
		break;
	case 1:
		if (RollChance(2))
			CUR_SCHED(npc).y = WORK_CHECK_LIGHT;
		else
			CUR_SCHED(npc).y = WORK_CHECK_SHUTTERS;
		Npc_pushSchedule(npc, GetChoice(npc), CUR_SCHED(npc).x, -1, 0);
		break;
	case 2:
		if (IsBlocked(npc))
			Npc_pushSchedule(npc, GetChoice(npc) == WORK_CHECK_LIGHT ? WORK_CHECK_SHUTTERS : WORK_CHECK_LIGHT,
				CUR_SCHED(npc).x, -1, 0);
		else {
			if (CUR_SCHED(npc).counter)
				SelectWeapon(*npc, 0, 0);
			Npc_popSchedule(npc, 0);
		}
		break;
	case 3:
		if (CUR_SCHED(npc).counter)
			SelectWeapon(*npc, 0, 0);
		Npc_popSchedule(npc, IsBlocked(npc) ? -1 : 0);
		break;
	}
}
