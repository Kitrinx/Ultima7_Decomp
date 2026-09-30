/* Black Gate U7.EXE, overlay segment 275 (file offsets 0x081270 to 0x0818bb, 1611 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "coord.h"
#include "u7npc.h"
#include "item.h"
#include "combatai.h"
#include "actitem.h"
#include "random.h"
#include "npcpath.h"
#include "usehook.h"
#include "script.h"

extern int16_t DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* the NPC's facing, and that facing turned by n eighths, as SCRIPT_FACE operands */
#define FACE_AHEAD(p) ((uint8_t) (NPC(p)->status & 7) + 48)
#define FACE_TURNED(p, n) (((uint8_t) (NPC(p)->status & 7) + (n)) % 8 + 48)

/* remember where the activity started */
inline void MarkStart(objref *p)
{
	NPC(p)->sVr[0] = Loc(Item_getX(*p)).value;
	NPC(p)->sVr[1] = Loc(Item_getY(*p)).value;
}

/* Strike poses and turn about on the spot, now and then taking a step or two nearby. */
void RunDanceSchedule(objref *npc)
{
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1);
		break;
	case 1:
		if (CanMove(npc)) {
			MarkStart(npc);
			CUR_SCHED(npc).state++;
		} else {
			Npc_setSchedule(npc, WORK_LOITER);
			CUR_SCHED(npc).state = 1;
		}
		break;
	case 2:
		PostScheduleScript(npc, MakeScript(SCRIPT_READY_FRAME, SCRIPT_READY_FRAME, SCRIPT_RAISE1_FRAME,
			SCRIPT_RAISE1_FRAME, SCRIPT_EXTEND1_FRAME, SCRIPT_EXTEND1_FRAME, SCRIPT_THRUST1_FRAME, SCRIPT_THRUST1_FRAME,
			SCRIPT_READY_FRAME, SCRIPT_READY_FRAME, SCRIPT_END));
		CUR_SCHED(npc).state = 8;
		break;
	case 3:
		PostScheduleScript(npc, MakeScript(SCRIPT_OUT_FRAME, SCRIPT_OUT_FRAME,
			SCRIPT_FACE, FACE_TURNED(npc, 2), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 4), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 6), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_AHEAD(npc), SCRIPT_WAIT, 2, SCRIPT_END));
		CUR_SCHED(npc).state = 8;
		break;
	case 4:
		PostScheduleScript(npc, MakeScript(SCRIPT_UP_FRAME, SCRIPT_UP_FRAME,
			SCRIPT_FACE, FACE_TURNED(npc, 2), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 4), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 6), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_AHEAD(npc), SCRIPT_WAIT, 2, SCRIPT_END));
		CUR_SCHED(npc).state = 8;
		break;
	case 5:
		PostScheduleScript(npc, MakeScript(SCRIPT_FACE, FACE_TURNED(npc, 2), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 4), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_TURNED(npc, 6), SCRIPT_WAIT, 2,
			SCRIPT_FACE, FACE_AHEAD(npc), SCRIPT_WAIT, 2, SCRIPT_END));
		CUR_SCHED(npc).state = 8;
		break;
	case 6:
		PostScheduleScript(npc, MakeScript(SCRIPT_UP_FRAME, SCRIPT_UP_FRAME, SCRIPT_OUT_FRAME, SCRIPT_OUT_FRAME,
			SCRIPT_LOOP, 0, GenerateRandomIntegerInRange(4) + 1, SCRIPT_END));
		CUR_SCHED(npc).state = 8;
		break;
	case 7:
		if (StartPath(*npc,
			GenerateRandomIntegerInRange(7) + NPC(npc)->sVr[0] - 3,
			GenerateRandomIntegerInRange(7) + NPC(npc)->sVr[1] - 3,
			0, 10, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else
			CUR_SCHED(npc).state = 9;
		break;
	case 8:
		ContinueScheduleWalk(npc);
		break;
	case 9:
		if (RollChance(8))
			RunUsable(0, *npc, -1);
		CUR_SCHED(npc).state = GenerateRandomIntegerInRange(6) + 2;
		break;
	}
}
