/* Black Gate U7.EXE, overlay segment 311 (file offsets 0x08fe00 to 0x0907b2, 2482 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "u7npc.h"
#include "item.h"
#include "coord.h"
#include "gtimer.h"
#include "random.h"
#include "npcpath.h"
#include "sprite.h"
#include "script.h"
#include "actitem.h"
#include "usehook.h"
#include "sortitem.h"
#include "search.h"
#include "text.h"

#define TYPE_LIGHT_SOURCE       336
#define TYPE_LIT_LIGHT_SOURCE   338
#define TYPE_LIT_LAMP           526
#define TYPE_LAMP_POST          889
#define TYPE_SPENT_LIGHT        997

/* TEXT.FLX lines 1123-1128, less the 1024 GetGameText adds for group 1 */
#define TEXT_BETTER             99  /* "Ah, 'tis better." and two more */
#define TEXT_CAN_SEE            102 /* "I can see again!" */
#define TEXT_NEED_NOT_BE_ON     103 /* "This need not be on." */
#define TEXT_NEEDS_REPLACING    104 /* "This needs replacing." */

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/*
 * Walk to the nearest lamp or light source that is in the wrong state for the hour, then use it:
 * lit ones go out by day, dark ones are lit by night, and spent ones are replaced.
 */
void far DoWorkCheckLight(objref *npc)
{
	AreaSearch ahead, lamps, sources, spent;
	int lampType, sourceType;
	Coord x, y;
	int dir;
	int best;
	int z;
	int distance;
	int frame;
	int range;
	int workType;
	objref lights[3];
	objref nearest;
	objref replacement;
	objref choice;
	int i;

	switch (CUR_SCHED(npc).state) {
	case 0:
		/* an argument of 1 asks to come back here afterwards */
		if (CUR_SCHED(npc).x == 1) {
			CUR_SCHED(npc).x = Item_getX(*npc);
			CUR_SCHED(npc).y = Item_getY(*npc);
		}
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		if (IsDaytime()) {
			lampType = TYPE_LIT_LAMP;
			sourceType = TYPE_LIT_LIGHT_SOURCE;
		} else {
			lampType = TYPE_LAMP_POST;
			sourceType = TYPE_LIGHT_SOURCE;
		}
		workType = NPC(npc)->workType;
		switch (workType) {
		case WORK_PREACH:
			range = 12;
			break;
		case WORK_SLEEP:
		case WORK_WAITER:
			range = 8;
			break;
		default:
			range = 12;
		}
		FindNearestItem(&lamps, x, y, range, 0, lampType, 0xff, 0xff);
		FindNearestItem(&sources, x, y, range, 0, sourceType, 0xff, 0xff);
		FindNearestItem(&spent, x, y, range, 0, TYPE_SPENT_LIGHT, 0xff, 0xff);
		/* a lit light source in frame 7 or 9 is passed over for a lamp or a spent light */
		lights[0] = objref(lamps);
		lights[1] = objref(sources);
		lights[2] = objref(spent);
		for (i = 0, best = 1000; i < 3; i++) {
			if (lights[i].valid()) {
				distance = Item_greatestDeltaToItem(*npc, lights[i]);
				if (distance < best) {
					best = distance;
					nearest = lights[i];
				}
			}
		}
		if (best == 1000) {
			Npc_popSchedule(npc, -1);
			break;
		}
		if (nearest.type() == TYPE_LIT_LIGHT_SOURCE && (nearest.frame() == 7 || nearest.frame() == 9)) {
			if (!lights[0].valid() && !lights[2].valid()) {
				Npc_popSchedule(npc, -1);
				break;
			}
			if (lights[0].valid() && !lights[2].valid())
				choice = lights[0];
			else if (!lights[0].valid() && lights[2].valid())
				choice = lights[2];
			else if (Item_greatestDeltaToItem(*npc, lights[0]) < Item_greatestDeltaToItem(*npc, lights[2]))
				choice = lights[0];
			else
				choice = lights[2];
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, choice.type(), -1);
		} else
			Npc_pushSchedule(npc, WORK_GOTO_ITEM, nearest.type(), -1);
		break;
	case 1:
		if (IsBlocked(npc))
			Npc_popSchedule(npc, -1);
		else
			PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 1, SCRIPT_READY_FRAME, SCRIPT_WAIT, 2,
				SCRIPT_END));
		break;
	case 2:
		x = Item_getX(*npc);
		y = Item_getY(*npc);
		dir = NPC(npc)->facing();
		replacement = 0;
		/* the light the NPC walked up to, one step ahead */
		FindItemInArea(&ahead, Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]),
			Coord(x + DirDeltaX[dir]), Coord(y + DirDeltaY[dir]), 0, NPC(npc)->result, 0xff, 0xff);
		if (ahead.found()) {
			/* a spent light is swapped for a fresh one; anything else is switched with a use */
			if (ahead.current.type() == TYPE_SPENT_LIGHT) {
				x = Item_getX(ahead.current);
				y = Item_getY(ahead.current);
				z = GetItemZAndStuff(&ahead.current).z();
				frame = ahead.current.frame();
				Item_delete(&ahead.current);
				CreateItem(&replacement, TypeFrame(IsDaytime() ? TYPE_LIGHT_SOURCE : TYPE_LIT_LIGHT_SOURCE),
					x, y, z);
				if (replacement.valid()) {
					Item_setFrame(&replacement, frame);
					Item_setQuality(&replacement, GenerateRandomIntegerInRange(27) + 30);
					Item_clearTemporary(&replacement);
					Item_clearOkayToTake(&replacement);
				}
			} else
				RunUsable(1, ahead.current, -1);
			if (RollChance(3))
				BarkLine(npc, GenerateRandomIntegerInRange(3) + TEXT_BETTER);
			else if (RollChance(3)) {
				switch ((unsigned) NPC(npc)->result) {
				case TYPE_LIGHT_SOURCE:
				case TYPE_LAMP_POST:
					BarkLine(npc, GenerateRandomIntegerInRange(1) + TEXT_CAN_SEE);
					break;
				case TYPE_LIT_LIGHT_SOURCE:
				case TYPE_LIT_LAMP:
					BarkLine(npc, GenerateRandomIntegerInRange(1) + TEXT_NEED_NOT_BE_ON);
					break;
				case TYPE_SPENT_LIGHT:
					BarkLine(npc, GenerateRandomIntegerInRange(1) + TEXT_NEEDS_REPLACING);
					break;
				}
			}
		}
		PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_WAIT, 15, SCRIPT_END));
		/* with nowhere to return to, finish now; else walk back, remembering whether a light was seen to */
		if (CUR_SCHED(npc).x == -1)
			Npc_popSchedule(npc, ahead.found() || replacement.valid() ? 0 : -1);
		else {
			if (StartPath(*npc, CUR_SCHED(npc).x, CUR_SCHED(npc).y, 0, 100, DiscardedPathLength, 0) == 0)
				CUR_SCHED(npc).x = ahead.found() || replacement.valid();
			else
				Npc_popSchedule(npc, -1);
		}
		break;
	case 3:
		ContinueScheduleWalk(npc);
		break;
	case 4:
		Npc_popSchedule(npc, CUR_SCHED(npc).x ? 0 : -1);
		break;
	}
}
