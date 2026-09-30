/* Black Gate U7.EXE, overlay segment 306 (file offsets 0x08e340 to 0x08e6b4, 884 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "actitem.h"
#include "search.h"
#include "actutil.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Fill the water trough: carry buckets from the well and pour them in until it shows full (frame 3). */
void DoWorkFillWaterTrough(objref *npc)
{
	objref bucket;
	AreaSearch trough;

	switch (CUR_SCHED(npc).state) {
	case 0:
		FindNearestItem(&trough, Item_getX(*npc), Item_getY(*npc), 25, 0, 719 /* water trough */, 0xff, 0xff);
		if (trough.found() && trough.current.frame() == 3)
			Npc_popSchedule(npc, 0);
		else
			CUR_SCHED(npc).state++;
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_FILL_BUCKET, 470 /* well */, -1);
		break;
	case 2:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, 719 /* water trough */, -1);
		break;
	case 3:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else {
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
			FindNearestItem(&trough, Item_getX(*npc), Item_getY(*npc), 5, 0, 719 /* water trough */, 0xff, 0xff);
			PostScriptToItem(&trough.current, MakeScript(SCRIPT_WAIT, 4, SCRIPT_SFX, 46, SCRIPT_NEXT_FRAME_MAX,
				SCRIPT_END));
		}
		break;
	case 4:
		bucket = FindCarriedItem(npc, 810 /* bucket */);
		if (bucket.valid())
			Item_setFrame(&bucket, 0);
		PostScheduleScript(npc, MakeScript(SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2,
			SCRIPT_END));
		FindNearestItem(&trough, Item_getX(*npc), Item_getY(*npc), 5, 0, 719 /* water trough */, 0xff, 0xff);
		if (trough.current.frame() < 3)
			CUR_SCHED(npc).state = 1;
		break;
	case 5:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 810 /* bucket */, -1);
		break;
	case 6:
		Npc_popSchedule(npc, 0);
		break;
	}
}
