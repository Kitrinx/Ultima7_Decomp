/* Black Gate U7.EXE, overlay segment 277 (file offsets 0x081c60 to 0x082142, 1250 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "activity.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sprite.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "text.h"
#include "script.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FACING(p) ((unsigned char) (NPC(p)->status & 7))

/* Farm: work the crops with a knife or scythe, barking now and then. */
void far RunFarmSchedule(objref *npc)
{
	AreaSearch crops;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_READY_HAND, RollChance(2) ? 615 /* knife */ : 618 /* scythe */, 1);
		break;
	case 2:
		NPC(npc)->scheduleValue = 0;
		CUR_SCHED(npc).state++;
		break;
	case 3:
		Npc_pushSchedule(npc, WORK_GOTO_ITEM,
			MakeTypeFrame(423 /* crops */, GenerateRandomIntegerInRange(8) * 4 + GenerateRandomIntegerInRange(3)), -1);
		break;
	case 4:
		if (IsBlocked(npc)) {
			/* "Whew!  'Tis hot!", "Work...work...work..." or "We need rain..." */
			if (RollChance(8))
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 96), 0, 15, 0);
			CUR_SCHED(npc).state = 3;
		} else if (RollChance(2))
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE1_FRAME, SCRIPT_WAIT, 1,
				SCRIPT_EXTEND1_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST1_FRAME, SCRIPT_WAIT, 2, SCRIPT_READY_FRAME,
				SCRIPT_END));
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE2_FRAME, SCRIPT_WAIT, 1,
				SCRIPT_EXTEND2_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST2_FRAME, SCRIPT_WAIT, 2, SCRIPT_READY_FRAME,
				SCRIPT_END));
		break;
	case 5:
		if (RollChance(4)) {
			FindItemInArea(&crops, Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
				Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]),
				Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)]),
				Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)]), 0, 423 /* crops */, 0xff, 0xff);
			/* crops come in groups of four frames; move these to the last of theirs */
			if (crops.found())
				Item_setFrame(&crops.current, crops.current.frame() | 3);
			CUR_SCHED(npc).state = 2;
		} else {
			if (NPC(npc)->scheduleValue >= 5) {
				/* "Blast!", "Ouch - cut myself!" or "These crops are tough!" */
				SpriteManager_barkOnItem(&gSpriteManager, *npc,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 63), 0, 15, 0);
				NPC(npc)->scheduleValue = 0;
			} else
				NPC(npc)->scheduleValue++;
			CUR_SCHED(npc).state = 4;
		}
		break;
	}
}
