/* Black Gate U7.EXE, overlay segment 295 (file offsets 0x08b680 to 0x08ba34, 948 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "actutil.h"
#include "random.h"
#include "actitem.h"
#include "search.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Desk work: sit at the nearer desk, or take a new desk item to one of the tables in reach. */
void far RunDeskWorkSchedule(objref *npc)
{
	int frame, pick;
	int x, y;
	int item;
	char found[4];
	unsigned char any;
	int i;
	AreaSearch desk1, desk2, place;
	int places[4] = { 407, 633, 1000, 283 };    /* desks and tables */

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		CUR_SCHED(npc).state = (GenerateRandomIntegerInRange(3) + 1) * 10;
		break;
	case 10:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		FindNearestItem(&desk1, x, y, 25, 0, 283 /* desk */, 0xff, 0xff);
		FindNearestItem(&desk2, x, y, 25, 0, 407 /* desk */, 0xff, 0xff);
		if (Item_greatestDeltaToItem(*npc, desk1.current.off) < Item_greatestDeltaToItem(*npc, desk2.current.off) ||
			!desk2.found())
			Npc_pushSchedule(npc, WORK_SIT, 283 /* desk */, -1);
		else
			Npc_pushSchedule(npc, WORK_SIT, 407 /* desk */, -1);
		break;
	case 11:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state = 1;
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, GenerateRandomIntegerInRange(31) + 60, SCRIPT_END));
		break;
	case 12:
		CUR_SCHED(npc).state = 1;
		break;
	case 20:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		for (i = 0, any = 0; i < 4; i++) {
			FindNearestItem(&place, x, y, 25, 0, places[i], 0xff, 0xff);
			if (place.found()) {
				found[i] = 1;
				any = 1;
			} else
				found[i] = 0;
		}
		if (!any)
			CUR_SCHED(npc).state = 1;
		else {
			frame = DeskItemFrames[GenerateRandomIntegerInRange(13)];
			item = MakeTypeFrame(675 /* desk item */, frame);
			do
				pick = GenerateRandomIntegerInRange(4);
			while (found[pick] == 0);
			Npc_pushSchedule(npc, WORK_MOVE_ITEM, item, places[pick]);
		}
		break;
	case 21:
		CUR_SCHED(npc).state = 1;
		break;
	case 30:
		Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1);
		break;
	case 31:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
