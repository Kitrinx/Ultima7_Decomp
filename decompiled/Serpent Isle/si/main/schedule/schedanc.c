/* Serpent Isle SI.EXE, overlay segment 259 (file offsets 0x06f8f0 to 0x070013, 1827 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "activity.h"
#include "coord.h"
#include "u7npc.h"
#include "npcref.h"
#include "item.h"
#include "combatai.h"
#include "actitem.h"
#include "random.h"
#include "npcpath.h"
#include "usehook.h"
#include "script.h"
#include "sprite.h"
#include "text.h"

/* TEXT.FLX lines 1243-1250, less the 1024 GetGameText adds for group 1 */
#define TEXT_DANCE_SONG     219 /* "Dee dee, dee da..." and seven more */

extern int DiscardedPathLength[2];

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

/* the NPC's facing, and that facing turned by n eighths, as SCRIPT_FACE operands */
#define FACE_AHEAD(p) ((unsigned char) (NPC(p)->status & 7) + 48)
#define FACE_TURNED(p, n) (((unsigned char) (NPC(p)->status & 7) + (n)) % 8 + 48)

/* remember where the activity started */
inline void MarkStart(objref *p)
{
	NPC(p)->sVr[0] = Loc(Item_getX(*p)).value;
	NPC(p)->sVr[1] = Loc(Item_getY(*p)).value;
}

/* Strike poses and turn about on the spot, now and then taking a step or two nearby. */
void far RunDanceSchedule(objref *npc)
{
	unsigned char singing = 0;
	objref iolo;

	/* in this stretch of the map, while Iolo's magic reads 13, dancers bark nonsense lines */
	GetNpcIbo(&iolo, 1);
	int x = Item_getX(*npc);
	int y = Item_getY(*npc);
	if (x > 0x350 && x < 0x480 && y > 0x660 && y < 0x7b0 && Npc_getMagic(&iolo) == 13)
		singing = 1;
	if (singing && RollChance(12))
		BarkLine(npc, GenerateRandomIntegerInRange(8) + TEXT_DANCE_SONG, 3);
	switch (CUR_SCHED(npc).state) {
	case 0:
		Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
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
			GenerateRandomIntegerInRange(7) + Npc_getScheduleVariable0(npc) - 3,
			GenerateRandomIntegerInRange(7) + Npc_getScheduleVariable1(npc) - 3,
			0, 10, DiscardedPathLength, 0) == 0)
			CUR_SCHED(npc).state++;
		else
			CUR_SCHED(npc).state = 9;
		break;
	case 8:
		ContinueScheduleWalk(npc);
		break;
	case 9:
		if (RollChance(8) && !singing)
			RunUsable(0, *npc, -1);
		if (NPC(npc)->workType == 4 /* dance */)
			CUR_SCHED(npc).state = GenerateRandomIntegerInRange(6) + 2;
		break;
	}
}
