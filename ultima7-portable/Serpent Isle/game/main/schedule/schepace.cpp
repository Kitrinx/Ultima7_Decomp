/* Serpent Isle SI.EXE, overlay segment 270 (file offsets 0x0739d0 to 0x074078, 1704 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "sprite.h"
#include "collide.h"
#include "legalmov.h"
#include "sortitem.h"
#include "itable.h"
#include "actqueue.h"
#include "random.h"
#include "actitem.h"
#include "text.h"
#include "script.h"
#include "search.h"
#include "voolook.h"
#include "monsters.h"

inline int8_t FindItemAt(AreaSearch *search, Loc x, Loc y, int16_t flags, int16_t type, int8_t quality, int16_t frame)
{
	return FindItemInArea(search, x, y, flags, type, quality, frame);
}

inline int8_t FindItemAt(AreaSearch *search, Loc x1, Loc y1, Loc x2, Loc y2, int16_t flags, int16_t type, int8_t quality,
	int16_t frame)
{
	return FindItemInArea(search, x1, y1, x2, y2, flags, type, quality, frame);
}

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* the direction the NPC paces in, kept in the first argument */
inline int16_t GetHeading(objref *p)
{
	return CUR_SCHED(p).x;
}

/* Pace back and forth, turning where the way is blocked and asking a blocking NPC to make way. */
void RunPaceSchedule(objref *npc)
{
	uint16_t move;
	int16_t monster;
	uint16_t type;
	AreaSearch blocker;

	type = objref(npc->off).type();
	monster = MonsterLookup.get(type);
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
		break;
	case 1:
		NPC(npc)->scheduleValue = 0;
		if (NPC(npc)->workType == WORK_HOR_PACE)
			CUR_SCHED(npc).x = 6;
		else
			CUR_SCHED(npc).x = 0;
		CUR_SCHED(npc).state++;
		break;
	case 2:
		if (NPC(npc)->scheduleValue == 0) {
			RemoveTypeFromCollision(*npc);
			move = CheckMove(Item_getX(*npc), Item_getY(*npc), Item_getZ(npc),
				ITEM(npc->off)->typeFrame, GetHeading(npc), 1, NPC(npc)->typeFlags);
			AddTypeToCollision(*npc);
			if ((move & 0x8000) && !(move & 0x2000))
				StepItem(*npc, GetHeading(npc), (int8_t) move, 1);
			else {
				FindItemAt(&blocker,
					Coord(Item_getX(*npc) + DirDeltaX[GetHeading(npc)]),
					Coord(Item_getY(*npc) + DirDeltaY[GetHeading(npc)]),
					4, -1, 0xff, 0xff);
				if (blocker.found()) {
					ActionQueue.add(*npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
					/* "Step aside!", "Stand away!" or "Move on!" */
					if (!(MonsterRecords.get(monster)->extraFlags & 0x20))
						SpriteManager_barkOnItem(&gSpriteManager, *npc,
							GetGameText(1, GenerateRandomIntegerInRange(3)), 0, 15, 0);
					NPC(npc)->scheduleValue++;
				} else
					CUR_SCHED(npc).state++;
			}
		} else if (NPC(npc)->scheduleValue > 15)
			NPC(npc)->scheduleValue = 0;
		else
			NPC(npc)->scheduleValue++;
		break;
	case 3:
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 2, SCRIPT_FACE, (GetHeading(npc) + 54) & 7,
			SCRIPT_WAIT, 2, SCRIPT_FACE, (GetHeading(npc) + 52) & 7, SCRIPT_WAIT, 2, SCRIPT_END));
		break;
	case 4:
		if (NPC(npc)->workType != WORK_HOR_PACE && NPC(npc)->workType != WORK_VER_PACE && RollChance(4))
			Npc_pushSchedule(npc, WORK_GOTO_REM_L, -1, -1, 0);
		else {
			CUR_SCHED(npc).x = (GetHeading(npc) + 4) & 7;
			CUR_SCHED(npc).state = 2;
		}
		break;
	case 5:
		Npc_popSchedule(npc, IsBlocked(npc) ? -1 : 0);
		break;
	}
}
