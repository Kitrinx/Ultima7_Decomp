/* Serpent Isle SI.EXE, overlay segment 351 (file offsets 0x0aaec0 to 0x0ab879, 2489 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: combaux.c */
#include "u7port.h"
#include "activity.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "item.h"
#include "collide.h"
#include "sprite.h"
#include "makemojo.h"
#include "combat.h"
#include "crime.h"
#include "monsters.h"
#include "sche_ov1.h"
#include "sortitem.h"
#include "itable.h"
#include "partymov.h"
#include "itemcmd.h"
#include "movepath.h"
#include "random.h"
#include "actitem.h"
#include "actutil.h"
#include "cast.h"
#include "cbattack.h"
#include "combpick.h"
#include "damage.h"
#include "missile.h"
#include "misstrac.h"
#include "search.h"
#include "sounds.h"
#include "npcpath.h"
#include "party.h"
#include "sche.h"
#include "slime.h"
#include "text.h"
#include "usehook.h"
#include "combmode.h"
#include "legalmov.h"
#include "type.h"
#include "npcref.h"
#include "coord.h"
#include "voolook.h"
#include "attack.h"
#include "ready.h"
#include "spell.h"
#include "weapons.h"
#include "mapview.h"
#include "combatai.h"
#include "wihh.h"

#define NPC(p) GetNpcBufferForIbo(p)
#define NPCNUM(p) (uint16_t)Item_getNpcNumber(p)
#define IN_PARTY(n) ((uint8_t) (((n)->status & NPC_IN_PARTY) != 0))
#define ALIGNMENT(n) ((uint8_t) (((n)->status & 0x18) >> 3))
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])
#define DEAD(p) ((uint8_t) ((int8_t)Item_getHitPoints(p) <= 0))

inline uint8_t HasOppressor(objref *p) { return NPC(p)->oppressor != -1; }

void RemoveFromCombat(NPCRef &self)
{
	int16_t i;
	int16_t align;
	int16_t work;
	NPCRef other;
	uint8_t active;
	uint8_t dead;
	uint8_t avatar;
	uint8_t unconscious;
	Coord x, y;
	int16_t npc;

	active = CanVisit(&self);
	dead = (NPC(&self)->status & NPC_DEAD) != 0;
	avatar = (uint8_t)Item_isAvatar(&self);
	npc = NPCNUM(&self);
	align = ALIGNMENT(NPC(&self));
	work = NPC(&self)->workType;
	unconscious = IsNpcUnconscious(&self);
	if (work != WORK_COMBAT && CUR_SCHED(&self).state == 0 && NPCNUM(&self) < 256 && !active) {
		Schedule_getCoord(&ScheduleTable, NPCNUM(&self), SchedulePeriod, &x, &y);
		if (work != WORK_WAIT) {
			if (GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef),
				Item_getZ(&AvatarRef), x, y, 0) >= 21
				&& MainWorldView.getChunkAt(Item_getX(self), Item_getY(self)) != MainWorldView.getChunkAt(x, y)) {
				Item_move(&self, x, y, 0);
			} else if (!PlaceNpcNearAvatar(&self, x, y, 1)) {
				Npc_setSchedule(&self, WORK_WANDER);
				CUR_SCHED(&self).state = 1;
			}
		}
	}
	if (CombatGroups.isProtectee(npc) && (dead || !active && !IN_PARTY(NPC(&self)))) {
		CombatGroups.clear(align);
		CombatGroups.releaseProtectors(align, NPC(&self)->oppressor);
	}
	if (IsInMode(&self, ATTACK_PROTECT) && work == WORK_COMBAT && (!active || dead))
		SetAttackMode(self, ATTACK_NEAREST);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (other.valid() && CanVisit(&other) && NPC(&other)->workType == WORK_COMBAT) {
			if (IsInMode(&other, ATTACK_PROTECT)) {
				if (NPC(&other)->primaryTarget == npc)
					ProtectLeader(&other);
				else if (NPC(&other)->secondaryTarget == npc)
					Npc_setTarget(&other, -1, 0);
			} else if (dead || avatar && unconscious) {
				if (NPC(&other)->primaryTarget == npc) {
					Npc_setTarget(&other, -1, 1);
					if ((uint8_t)Item_isAvatar(&other))
						KillNpcMode = 0;
					if (!IsInMode(&other, ATTACK_FLEE) && !IsInMode(&other, ATTACK_PROTECT) && !DEAD(&other)
						&& (uint8_t) (NPC(&other)->typeFlagsHigh & 1) && !(uint8_t)Item_isAvatar(&other)
						&& !ChooseTarget(other, 100) && HasSecondary(&other))
						NPC(&other)->typeFlagsHigh = NPC(&other)->typeFlagsHigh & ~1;
				} else if (NPC(&other)->secondaryTarget == npc) {
					Npc_setTarget(&other, -1, 0);
					NPC(&other)->typeFlagsHigh = NPC(&other)->typeFlagsHigh | 1;
				}
			}
			if (NPC(&other)->oppressor == npc && (!active || dead || avatar && unconscious)) {
				if (IsInMode(&other, ATTACK_FLEE)) {
					NPC(&other)->oppressor = ChooseNearestTarget(&other, 1, 0, 100);
					if (!HasOppressor(&other))
						NPC(&other)->oppressor = ChooseNearestTarget(&other, 1, 1, 100);
				} else
					NPC(&other)->oppressor = -1;
			}
		}
	}
	NPC(&self)->primaryTarget = -1;
	NPC(&self)->secondaryTarget = -1;
	NPC(&self)->typeFlagsHigh = NPC(&self)->typeFlagsHigh | 1;
	if (dead || DEAD(&self) || !active)
		NPC(&self)->oppressor = -1;
	StopPaths(self);
	if (dead)
		CombatGroups.movePoints[npc] = 0;
	if ((align == 2 || align == 3) && work == WORK_COMBAT && CombatGroups.countEnemies() == 0)
		CombatGroups.playEndMusic();
}

uint8_t PathNextToItem(objref *npc, NPCRef target, int16_t limit)
{
	int8_t result;
	Coord x, y;
	int16_t z;
	int16_t length;

	if (!FindSpotNextToItem(npc, target, &x, &y, &z))
		return 0;
	result = StartPath(*npc, x, y, z, limit, &length, 1);
	if (result == 0) {
		if (WalkNPCRoute(*npc, 1) == 2)
			return 0;
		StopPaths(*npc);
		return 1;
	}
	if (result == 1)
		return 1;
	return 0;
}

uint8_t IsSentient(NPCRef &npc)
{
	int16_t monster = MonsterLookup.get(GetItemType(npc));

	return (uint8_t) MonsterRecords.get(monster)->intelligence >= 6;
}

uint8_t GetDirectionTo(objref *from, objref to)
{
	return DirectionBySign[GetSign(Item_getX(to) - Item_getX(*from)) + 1]
		[GetSign(Item_getY(to) - Item_getY(*from)) + 1];
}
