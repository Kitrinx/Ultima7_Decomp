/* Serpent Isle SI.EXE, overlay segment 273 (file offsets 0x076030 to 0x076b26, 2806 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "usehook.h"
#include "actutil.h"
#include "search.h"
#include "script.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FACING(p) ((uint8_t) (NPC(p)->status & 7))

/* Sew: spin wool into thread, weave the thread into cloth at the loom, then cut the cloth into clothes. */
void RunSewSchedule(objref *npc)
{
	objref created;
	Coord x, y;
	uint8_t z;
	AreaSearch found, unusedSearch;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		if (RollChance(3)) {
			CUR_SCHED(npc).state = 0;
			Npc_pushSchedule(npc, WORK_CHECK_AREA, 1, -1, 0);
		} else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, 653 /* bale of wool */, -1, 0);
		break;
	case 2:
		if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			Npc_pushSchedule(npc, WORK_DROP_ITEM, 653 /* bale of wool */, -1, 0);
		break;
	case 3:
		Npc_pushSchedule(npc, WORK_SIT, 651 /* spinning wheel */, -1, 0);
		break;
	case 4:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			CUR_SCHED(npc).state++;
		break;
	case 5:
		PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 40, SCRIPT_END));
		FindNearestItem(&found, Item_getX(*npc), Item_getY(*npc), 5, 0, 651 /* spinning wheel */, 0xff, 0xff);
		PostScriptToItem(&found.current, MakeScript(SCRIPT_SFX, 100, SCRIPT_CONTINUE, SCRIPT_NEXT_FRAME,
			SCRIPT_LOOP, 0, 39, SCRIPT_END));
		break;
	case 6:
		CreateCarriedItem(654 /* spindle of thread */, *npc);
		CUR_SCHED(npc).state++;
		break;
	case 7:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 654 /* spindle of thread */, -1, 0);
		break;
	case 8:
		if (IsBlocked(npc))
			CUR_SCHED(npc).state++;
		else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, 654 /* spindle of thread */, -1, 0);
		break;
	case 9:
		if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, 261 /* loom */, -1, 0);
		break;
	case 10:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else {
			FindNearestItem(&found, Item_getX(*npc), Item_getY(*npc), 5, 0, 261 /* loom */, 0xff, 0xff);
			PostScriptToItem(&found.current, MakeScript(SCRIPT_SFX, 73, SCRIPT_CONTINUE, SCRIPT_NEXT_FRAME,
				SCRIPT_NEXT_FRAME, SCRIPT_NEXT_FRAME, SCRIPT_NEXT_FRAME, SCRIPT_NEXT_FRAME, SCRIPT_NEXT_FRAME,
				SCRIPT_NEXT_FRAME, SCRIPT_NEXT_FRAME, SCRIPT_LOOP, 0, 6, SCRIPT_END));
			PostScheduleScript(npc, MakeScript(SCRIPT_WAIT, 52, SCRIPT_END));
		}
		break;
	case 11:
		CreateCarriedItem(MakeTypeFrame(851 /* cloth */, GenerateRandomIntegerInRange(5)), *npc);
		CUR_SCHED(npc).state++;
		created = FindCarriedItem(npc, 654 /* spindle of thread */);
		if (created.valid())
			Item_delete(&created);
		break;
	case 12:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 851 /* cloth */, -1, 0);
		break;
	case 13:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 851 /* cloth */, 971 /* table */, 0);
		break;
	case 14:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			Npc_pushSchedule(npc, WORK_READY_HAND, 698 /* shears */, 1, 0);
		break;
	case 15:
		if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, 851 /* cloth */, -1, 0);
		break;
	case 16:
		if (IsBlocked(npc)) {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		} else
			PostScheduleScript(npc, MakeScript(SCRIPT_RAISE1_FRAME, SCRIPT_RAISE1_FRAME, SCRIPT_EXTEND1_FRAME,
				SCRIPT_EXTEND1_FRAME, SCRIPT_THRUST1_FRAME, SCRIPT_THRUST1_FRAME, SCRIPT_RAISE1_FRAME,
				SCRIPT_RAISE1_FRAME, SCRIPT_EXTEND1_FRAME, SCRIPT_EXTEND1_FRAME, SCRIPT_THRUST1_FRAME,
				SCRIPT_THRUST1_FRAME, SCRIPT_END));
		break;
	case 17:
		FindItemInArea(&found,
			Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
			Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]),
			Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
			Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]),
			0, 851 /* cloth */, 0xff, 0xff);
		if (found.found()) {
			created = CreateCarriedItem(403 /* cloak */, *npc);
			x = Item_getX(found.current);
			y = Item_getY(found.current);
			z = Item_getZ(&found.current);
			Item_delete(&found.current);
			Item_move(&created, x, y, z);
			created = CreateCarriedItem(MakeTypeFrame(851 /* cloth */, GenerateRandomIntegerInRange(5) + 5), *npc);
			CUR_SCHED(npc).state++;
		} else {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 18:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 851 /* cloth */, -1, 0);
		break;
	case 19:
		if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, 403 /* cloak */, -1, 0);
		break;
	case 20:
		Npc_pushSchedule(npc, WORK_MOVE_ITEM, 403 /* cloak */, RollChance(2) ? 971 /* table */ : 890 /* table */, 0);
		break;
	case 21:
		if (IsBlocked(npc)) {
			while (DeleteCarriedItem(npc, 403 /* cloak */))
				;
		}
		if (RollChance(10))
			RunUsable(0, *npc, -1);
		else
			Npc_pushSchedule(npc, WORK_GRAB_ITEM, 851 /* cloth */, -1, 0);
		break;
	case 22:
		DeleteCarriedItem(npc, 851 /* cloth */);
		CUR_SCHED(npc).state++;
		break;
	case 23:
		Npc_pushSchedule(npc, WORK_DROP_ITEM, 698 /* shears */, 890 /* table */, 0);
		break;
	case 24:
		CUR_SCHED(npc).state = 1;
		break;
	}
}
