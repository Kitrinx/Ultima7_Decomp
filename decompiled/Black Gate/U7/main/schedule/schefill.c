/* Black Gate U7.EXE, overlay segment 302 (file offsets 0x08cd50 to 0x08d1eb, 1179 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "actutil.h"
#include "actitem.h"
#include "search.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* Draw water: fetch the bucket, walk to the trough or the well, and work it. */
void far DoWorkFillBucket(objref *npc)
{
	objref bucket;
	AreaSearch source;
	int type;

	switch (CUR_SCHED(npc).state) {
	case 0:
		bucket = FindCarriedItem(npc, 810 /* bucket */);
		if (bucket.valid()) {
			/* this reads the empty search, not the bucket */
			if (source.current.frame() == 1)
				Npc_popSchedule(npc, 0);
			else
				CUR_SCHED(npc).state++;
		} else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, 810 /* bucket */, 1);
		break;
	case 1:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else {
			if (CUR_SCHED(npc).x == 719 /* water trough */)
				type = 719 /* water trough */;
			else if (CUR_SCHED(npc).x == 470 /* well */)
				type = 470 /* well */;
			else {
				/* an empty trough sends the NPC to the well */
				type = 719 /* water trough */;
				FindNearestItem(&source, Item_getX(*npc), Item_getY(*npc), 25, 0, 719 /* water trough */, 0xff, 0xff);
				if (!source.found() || source.current.frame() == 0)
					type = 470 /* well */;
			}
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, type, -1);
		}
		break;
	case 2:
		if (IsBlocked(npc)) {
			bucket = FindCarriedItem(npc, 810 /* bucket */);
			if (bucket.valid())
				goto fill;
			Npc_popSchedule(npc, 0);
		} else if (NPC(npc)->result == 719 /* water trough */) {
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_BEND_FRAME, SCRIPT_WAIT, 2, SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_END));
			FindNearestItem(&source, Item_getX(*npc), Item_getY(*npc), 25, 0, 719 /* water trough */, 0xff, 0xff);
			PostScriptToItem(&source.current, MakeScript(SCRIPT_WAIT, 5, SCRIPT_PREV_FRAME_MIN, SCRIPT_END));
		} else {
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_READY_FRAME, SCRIPT_THRUST1_FRAME,
				SCRIPT_THRUST1_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_EXTEND1_FRAME,
				SCRIPT_EXTEND1_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_THRUST1_FRAME,
				SCRIPT_THRUST1_FRAME, SCRIPT_READY_FRAME, SCRIPT_READY_FRAME, SCRIPT_END));
			FindNearestItem(&source, Item_getX(*npc), Item_getY(*npc), 25, 0, 740 /* well */, 0xff, 0xff);
			PostScriptToItem(&source.current, MakeScript(SCRIPT_FRAME, 1, SCRIPT_FRAME, 1, SCRIPT_FRAME, 2,
				SCRIPT_FRAME, 2, SCRIPT_FRAME, 3, SCRIPT_WAIT, 5, SCRIPT_FRAME, 4, SCRIPT_FRAME, 4, SCRIPT_FRAME, 5,
				SCRIPT_FRAME, 5, SCRIPT_FRAME, 0, SCRIPT_END));
		}
		break;
	case 3:
		/* frame 1 is a full bucket */
		bucket = FindCarriedItem(npc, 810 /* bucket */);
		if (bucket.valid())
	fill:
			Item_setFrame(&bucket, 1);
		Npc_popSchedule(npc, 0);
		break;
	}
}
