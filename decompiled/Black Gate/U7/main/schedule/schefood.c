/* Black Gate U7.EXE, overlay segment 276 (file offsets 0x081940 to 0x081c23, 739 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "script.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Eat: put food on a plate, sit at it, wait a while, then eat the food ahead. */
void far RunEatSchedule(objref *npc)
{
	AreaSearch food;
	int x, y;
	int dir;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 377 /* food item */, 717 /* plate */);
		break;
	case 2:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			CUR_SCHED(npc).state++;
		break;
	case 3:
		Npc_pushSchedule(npc, WORK_SIT, 717 /* plate */, -1);
		break;
	case 4:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			CUR_SCHED(npc).state++;
		break;
	case 5:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(31) + 20, SCRIPT_END));
		break;
	case 6:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		dir = (unsigned char) (NPC(npc)->status & 7);
		FindItemInArea(&food, Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
			Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]), 0, 377 /* food item */, 0xff, 0xff);
		if (food.found())
			Item_delete(&food.current);
		CUR_SCHED(npc).state++;
		break;
	}
}
