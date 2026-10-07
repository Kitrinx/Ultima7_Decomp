/* Serpent Isle SI.EXE, overlay segment 257 (file offsets 0x06f410 to 0x06f645, 565 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "random.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Duel: practice archery or fencing, at whichever of a target or a dummy lies within 25 cells. */
void RunDuelSchedule(objref *npc)
{
	AreaSearch target, dummy;
	Coord x, y;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		if (RollChance(2) && FindItemInArea(&target, Coord(x - 25), Coord(y - 25), Coord(x + 25),
			Coord(y + 25), 0, 735 /* archery target */, 0xff, 0xff)) {
			Npc_pushSchedule(npc, WORK_ARCHERY, -1, -1, 0);
			break;
		}
		if (FindItemInArea(&dummy, Coord(x - 25), Coord(y - 25), Coord(x + 25), Coord(y + 25),
			0, 860 /* fencing dummy */, 0xff, 0xff))
			Npc_pushSchedule(npc, WORK_FENCING, -1, -1, 0);
		break;
	case 2:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
