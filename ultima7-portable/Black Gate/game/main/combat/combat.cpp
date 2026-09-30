/* Black Gate U7.EXE, overlay segment 218 (file offsets 0x05b1e0 to 0x05b4ed, 781 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "activity.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "u7npc.h"
#include "u7event.h"
#include "combatai.h"
#include "mouse.h"
#include "partymov.h"
#include "barge.h"
#include "combpick.h"
#include "combmode.h"
#include "combat.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define IS_VALID(r) ((int8_t) ((r).off != 0))
#define IN_PARTY(n) ((int8_t) (((n)->status & NPC_IN_PARTY) != 0))
#define IS_CONJURED(n) ((uint8_t) ((n)->typeFlagsHigh & 0x40))

void ClearPartyMissileFlags(void)
{
	int16_t i;

	for (i = 0; i < 8; i++)
		PartyMissileFlags[i] = 0;
}

void BeginCombat(void)
{
	objref *party = PartyMembers;
	int16_t count = PartySize;
	int16_t i;
	objref npc;
	NPCRef target;
	objref member, leader;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (IS_VALID(npc) && (IN_PARTY(NPC(&npc)) || IS_CONJURED(NPC(&npc)))
			&& NPC(&npc)->workType != WORK_WAIT && NPC(&npc)->workType != WORK_COMBAT) {
			Npc_setSchedule(&npc, WORK_COMBAT);
			CUR_SCHED(&npc).state = 1;
		}
	}
	if (!GameInput.isGumpMode())
		SelectMouseCursor(32);
	LeaveVehicle();
	ClearPartyMissileFlags();
	if ((int8_t) (CombatGroups.leaders[1] != -1)) {
		GetNpcIbo(&leader, CombatGroups.leaders[1]);
		for (i = 0; i < NPC_COUNT; i++) {
			GetNpcIbo(&member, FindLeaderAttacker(&leader));
			if (IS_VALID(member) == 0)
				return;
			GetNpcIbo(&target, FindNearestNpc(&member, Item_getX(member),
				Item_getY(member), 1, ATTACK_PROTECT, 0));
			if (IS_VALID(target) == 0)
				return;
			Npc_setTarget(&target, Item_getNpcNumber(&member), 1);
		}
	}
}

void BreakOffCombat(void)
{
	objref *party = PartyMembers;
	int16_t count = PartySize;
	int16_t i;
	objref npc;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (IS_VALID(npc) && (IN_PARTY(NPC(&npc)) || IS_CONJURED(NPC(&npc)))
			&& NPC(&npc)->workType != WORK_WAIT) {
			Npc_setSchedule(&npc, WORK_FOLLOW_AVT);
			CUR_SCHED(&npc).state = 1;
		}
	}
	if (!GameInput.isGumpMode())
		SelectMouseCursor(8);
	CombatGroups.partyAttacked = 0;
	if (CombatGroups.countEnemies() == 0)
		CombatGroups.battleMusic = 0;
}
