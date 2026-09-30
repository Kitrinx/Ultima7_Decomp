/* Black Gate U7.EXE, overlay segment 278 (file offsets 0x0821a0 to 0x082927, 1927 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "typefram.h"
#include "iteminfo.h"
#include "activity.h"
#include "coord.h"
#include "u7manage.h"
#include "item.h"
#include "u7npc.h"
#include "sprite.h"
#include "sortitem.h"
#include "random.h"
#include "actitem.h"
#include "actutil.h"
#include "text.h"
#include "script.h"
#include "search.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define FACING(p) ((uint8_t) (NPC(p)->status & 7))
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define VALID(n) ((int8_t) ((n) != 0))

/*
 * Mine: take up a pick and hack at lead or iron ore. Now and then the rock ahead cracks a frame
 * further; an uncracked one may instead give way to a gold nugget or a gem.
 */
void RunMinerSchedule(objref *npc)
{
	int16_t created;
	Coord x, y;
	int16_t z;
	AreaSearch rock;

	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		Npc_pushSchedule(npc, WORK_READY_HAND, 624 /* pick */, 1);
		break;
	case 2:
		CUR_SCHED(npc).x = 0;
		CUR_SCHED(npc).state++;
		break;
	case 3:
		CUR_SCHED(npc).y = RollChance(2) ? 915 /* chunks of lead */ : 916 /* chunks of iron ore */;
		Npc_pushSchedule(npc, WORK_GOTO_ITEM, MakeTypeFrame(CUR_SCHED(npc).y, 0), -1);
		break;
	case 4:
		if (IsBlocked(npc)) {
			if (RollChance(5)) {
				Npc_setSchedule(npc, WORK_LOITER);
				CUR_SCHED(npc).state = 1;
			} else
				CUR_SCHED(npc).state = 3;
		} else if (RollChance(2))
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE1_FRAME,
				SCRIPT_WAIT, 1, SCRIPT_EXTEND1_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST1_FRAME, SCRIPT_SFX, 4,
				SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_END));
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_WAIT, 1, SCRIPT_RAISE2_FRAME,
				SCRIPT_WAIT, 1, SCRIPT_EXTEND2_FRAME, SCRIPT_WAIT, 1, SCRIPT_THRUST2_FRAME, SCRIPT_SFX, 4,
				SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_END));
		break;
	case 5:
		if (RollChance(4)) {
			x = Coord(Item_getX(*npc) + DirDeltaX[FACING(npc)] * 3);
			y = Coord(Item_getY(*npc) + DirDeltaY[FACING(npc)] * 3);
			FindItemInArea(&rock, x, y, x, y, 0, CUR_SCHED(npc).y, 0xff, 0xff);
			if (rock.found()) {
				if (FRAME(ITEM(rock.current.off)) == 0 && RollChance(10)) {
					x = Item_getX(rock.current);
					y = Item_getY(rock.current);
					z = Item_getZ(&rock.current);
					Item_delete(&rock.current);
					created = CreateCarriedItem(RollChance(2) ? 645 /* gold nugget */ : 760 /* gem */, npc->off);
					if (VALID(created))
						Item_move((objref *) &created, x, y, z);
					/* "Eureka!", "I am rich!" or "I must be worthy!" */
					if (RollChance(2))
						SpriteManager_barkOnItem(&gSpriteManager, npc->off,
							GetGameText(1, GenerateRandomIntegerInRange(3) + 69), 0, 15, 0);
					CUR_SCHED(npc).state = 2;
					return;
				}
				Item_setFrame(&rock.current,
					ClampShapeFrame(TYPE(ITEM(rock.current.off)), FRAME(ITEM(rock.current.off)) + 1));
				if (FRAME(ITEM(rock.current.off)) == 3) {
					CUR_SCHED(npc).state = 2;
					return;
				}
				CUR_SCHED(npc).x = 0;
			} else {
				CUR_SCHED(npc).state = 2;
				return;
			}
		} else {
			if ((uint16_t) CUR_SCHED(npc).x >= 5) {
				/* "Still no gold!", "Whew!" or "Am I not worthy?" */
				SpriteManager_barkOnItem(&gSpriteManager, npc->off,
					GetGameText(1, GenerateRandomIntegerInRange(3) + 66), 0, 15, 0);
				CUR_SCHED(npc).x = 0;
			} else
				CUR_SCHED(npc).x++;
		}
		CUR_SCHED(npc).state = 4;
	}
}
