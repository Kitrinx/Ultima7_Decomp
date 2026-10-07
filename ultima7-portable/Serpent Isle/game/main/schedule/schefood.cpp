/* Serpent Isle SI.EXE, overlay segment 260 (file offsets 0x0700a0 to 0x07038c, 748 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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
void RunEatSchedule(objref *npc)
{
	AreaSearch food;
	int16_t x, y;
	int16_t dir;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 377 /* food item */, 717 /* plate */, 0);
		break;
	case 2:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			CUR_SCHED(npc).state++;
		break;
	case 3:
		Npc_pushSchedule(npc, WORK_SIT, 717 /* plate */, -1, 0);
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
		dir = (uint8_t) (NPC(npc)->status & 7);
		FindItemInArea(&food, Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
			Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]), 0, 377 /* food item */, 0xff, 0xff);
		if (food.found())
			Item_delete(&food.current);
		CUR_SCHED(npc).state++;
		break;
	}
}
