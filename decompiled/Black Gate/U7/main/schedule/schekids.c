/* Black Gate U7.EXE, overlay segment 283 (file offsets 0x085070 to 0x085237, 455 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "random.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Kid games: play tag, over and over; the only game on the list. */
void far RunKidGamesSchedule(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(1) + 1) * 10;
		break;
	case 2:
		if (IsBlocked(npc)) {
			Npc_pushSchedule(npc, WORK_LOITER, -1, -1);
			CUR_SCHED(npc).state = 1;
		} else
			CUR_SCHED(npc).state = 1;
		break;
	case 3:
		CUR_SCHED(npc).state = 1;
		break;
	case 10:
		Npc_pushSchedule(npc, WORK_TAG, -1, -1);
		break;
	case 11:
		CUR_SCHED(npc).state = 2;
		break;
	}
}
