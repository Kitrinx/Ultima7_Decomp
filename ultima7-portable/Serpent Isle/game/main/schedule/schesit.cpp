/* Serpent Isle SI.EXE, overlay segment 280 (file offsets 0x079f30 to 0x07a217, 743 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "u7npc.h"
#include "party.h"

/* the bed usecode sends the avatar to */
int16_t NapBed;
/* the seat usecode chose for each party member */
/* SI.EXE kept usehook's LastUsecodeFunction right after the array: index 8, GetPartyIndex's answer
 * for a non-member of a full party, lands there. */
static struct {
	int16_t refs[8];
	uint16_t lastUsecodeFunction;
} PartySitArea;
int16_t (&PartySitRefs)[8] = PartySitArea.refs;
uint16_t &LastUsecodeFunction = PartySitArea.lastUsecodeFunction;

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define IN_PARTY(n) ((int8_t) (((n)->status & NPC_IN_PARTY) != 0))

/* The sitting schedule; a party member that cannot stay seated rejoins the avatar. */
void RunMajorSitSchedule(objref *npc)
{
	if (IN_PARTY(NPC(npc))) {
		switch (CUR_SCHED(npc).state) {
		case 1:
			if (PartySitRefs[GetPartyIndex(*npc)])
				Npc_pushSchedule(npc, WORK_HACK_SIT, -1, -1, 0);
			else
				CUR_SCHED(npc).state = 5;
			break;
		case 2:
			if (IsBlocked(npc))
				CUR_SCHED(npc).state = 5;
			else
				CUR_SCHED(npc).state++;
			break;
		case 5:
			Npc_pushSchedule(npc, WORK_SIT, -1, -1, 0);
			break;
		case 6:
			if (IsBlocked(npc)) {
				if ((int8_t) Item_isAvatar(npc))
					SetPartyWorkType(WORK_FOLLOW_AVT);
				else {
					Npc_setSchedule(npc, WORK_FOLLOW_AVT);
					CUR_SCHED(npc).state = 1;
				}
			} else
				CUR_SCHED(npc).state++;
			break;
		}
	} else {
		switch (CUR_SCHED(npc).state) {
		case 0:
			Npc_pushSchedule(npc, WORK_GOTO_SCH_D, -1, -1, 0);
			break;
		case 1:
			Npc_pushSchedule(npc, WORK_SIT, -1, -1, 0);
			break;
		case 2:
			if (IsBlocked(npc)) {
				Npc_setSchedule(npc, WORK_LOITER);
				CUR_SCHED(npc).state = 1;
			} else
				CUR_SCHED(npc).state++;
			break;
		}
	}
}

extern "C" void ResetSchesitGlobals(void)
{
	NapBed = 0;
	memset(PartySitRefs, 0, sizeof(PartySitRefs));
}
