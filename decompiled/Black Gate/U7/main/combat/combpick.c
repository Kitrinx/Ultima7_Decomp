/* Black Gate U7.EXE, overlay segment 221 (file offsets 0x060790 to 0x0620fd, 6509 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "voolook.h"
#include "u7npc.h"
#include "makemojo.h"
#include "sprite.h"
#include "combat.h"
#include "combatai.h"
#include "monsters.h"
#include "random.h"
#include "missile.h"
#include "text.h"
#include "combmode.h"
#include "mapview.h"
#include "combpick.h"

#define NPC(p) GetNpcBufferForIbo(p)

/* Whether self, a monster of the given type, may attack other within distance. */
#define CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance) \
	(other.valid() && CanVisit(&other) && AreEnemies(self, other) && !IsDead(&other) && \
		(allowHelpless || !IsNpcUnconscious(&other) && !IsInMode(&other, ATTACK_FLEE)) && \
		(allowTargeted || !IsTargeting(self, other)) && \
		(!(unsigned char)(Item_getQualityFlags(&other) & QUALITY_INVISIBLE) || \
			(unsigned char)MonsterRecords.get(type)->seeInvisible) && \
		Item_greatestDeltaToItem(*self, other) <= distance)

/* good, evil and chaotic are each hostile to the other two; neutrals to none */
unsigned char far AreEnemies(objref *self, objref other)
{
	int theirs = GetAlignment(&other);

	switch (GetAlignment(self)) {
	case 1:
		if (theirs == 3 || theirs == 2)
			return 1;
		break;
	case 2:
		if (theirs == 3 || theirs == 1)
			return 1;
		break;
	case 3:
		if (theirs == 2 || theirs == 1)
			return 1;
		break;
	}
	return 0;
}

void far RallyProtectors(NPCRef &self)
{
	NPCRef npc, target;
	objref member;
	int count;
	int i;

	for (i = 0, count = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&npc, i);
		if (npc.valid() && CanVisit(&npc) && !IsNpcUnconscious(&npc) &&
			GetAlignment(&npc) == GetAlignment(&self) && IsSentient(npc) &&
			!IsInMode(&npc, ATTACK_FLEE) && !IsInMode(&npc, ATTACK_BERSERK) &&
			ProtectChance[GetAlignment(&self)] > GenerateRandomIntegerInRange(100) &&
			!(unsigned char)Item_isAvatar(&npc) && (!IsInParty(&npc) || IsAvatarInCombat()) &&
			(HasLineOfFire(self, npc) || Item_greatestDeltaToItem(self, npc) <= 25)) {
			SetAttackMode(npc, ATTACK_PROTECT);
			count++;
		}
	}
	if (count != 0) {
		CombatGroups.setLeader(Item_getNpcNumber(&self));
		SpriteManager_barkOnItem(&gSpriteManager, self,
			GetGameText(1, GenerateRandomIntegerInRange(4) + 48), 5, 15, 0);
		for (i = 0; i < NPC_COUNT; i++) {
			GetNpcIbo(&member, FindLeaderAttacker(&self));
			if (member.valid() == 0)
				return;
			GetNpcIbo(&target, FindNearestNpc(&member, Item_getX(member), Item_getY(member),
				GetAlignment(&self), ATTACK_PROTECT, 0));
			if (target.valid() == 0)
				return;
			Npc_setTarget(&target, Item_getNpcNumber(&member), 1);
		}
	}
}

unsigned char far FindGroupCentre(objref *self, Coord *x, Coord *y, unsigned char hostile)
{
	int count = 0;
	long sumX = 0, sumY = 0;
	objref other;
	int i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (other.valid() && CanVisit(&other) &&
			(hostile && AreEnemies(self, other) || !hostile && GetAlignment(&other) == GetAlignment(self))) {
			sumX += Item_getX(other);
			sumY += Item_getY(other);
			count++;
		}
	}
	if (count == 0)
		return 0;
	*x = sumX / count;
	*y = sumY / count;
	return 1;
}

int far ChooseRandomTarget(objref *self, char allowTargeted, char allowHelpless, int distance)
{
	int count = 0, pick;
	objref other;
	int type;
	int i;

	type = GetMonsterNumber(*self);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance))
			count++;
	}
	if (count == 0)
		return -1;
	do {
		if (count == 2 && IsNpcUnconscious(&AvatarRef))
			pick = 1;
		else
			pick = GenerateRandomIntegerInRange(count);
		for (i = 0; i < NPC_COUNT && pick != -1; i++) {
			GetNpcIbo(&other, i);
			if (CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance))
				pick--;
		}
	} while (count != 1 && i == 1 && IsNpcUnconscious(&AvatarRef));
	return i - 1;
}

int far ChooseNearestTarget(objref *self, char allowTargeted, char allowHelpless, int distance)
{
	int best = 1000, d, nearest = -1;
	objref other, current;
	int type;
	int i;

	type = GetMonsterNumber(*self);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance)) {
			d = Item_greatestDeltaToItem(*self, other);
			if (d < best || nearest == 0 && IsNpcUnconscious(&AvatarRef)) {
				best = d;
				nearest = i;
			} else {
				GetNpcIbo(&current, nearest);
				if (d == best && allowTargeted && IsTargeting(self, other) && !IsTargeting(self, current)) {
					best = d;
					nearest = i;
				} else if (d == best && allowTargeted && IsTargeting(self, other) && IsTargeting(self, current) &&
					Item_getNpcNumber(&other) == NPC(self)->primaryTarget) {
					best = d;
					nearest = i;
				}
			}
		}
	}
	return nearest;
}

int far ChooseWeakestTarget(objref *self, char allowTargeted, char allowHelpless, int distance)
{
	int weakest = -1, health = 127;
	objref other, current;
	int type;
	int i;

	type = GetMonsterNumber(*self);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance)) {
			if ((char)Item_getHitPoints(&other) < health || weakest == 0 && IsNpcUnconscious(&AvatarRef)) {
				health = (char)Item_getHitPoints(&other);
				weakest = i;
			} else {
				GetNpcIbo(&current, weakest);
				if ((char)Item_getHitPoints(&other) == health && allowTargeted && IsTargeting(self, other) &&
					!IsTargeting(self, current)) {
					health = (char)Item_getHitPoints(&other);
					weakest = i;
				} else if ((char)Item_getHitPoints(&other) == health && allowTargeted && IsTargeting(self, other) &&
					IsTargeting(self, current) &&
					Item_getNpcNumber(&other) == NPC(self)->primaryTarget) {
					health = (char)Item_getHitPoints(&other);
					weakest = i;
				}
			}
		}
	}
	return weakest;
}

int far ChooseStrongestTarget(objref *self, char allowTargeted, char allowHelpless, int distance)
{
	int strongest = -1, health = -127;
	objref other, current;
	int type;
	int i;

	type = GetMonsterNumber(*self);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&other, i);
		if (CAN_ATTACK(self, other, allowTargeted, allowHelpless, type, distance)) {
			if ((char)Item_getHitPoints(&other) > health || strongest == 0 && IsNpcUnconscious(&AvatarRef)) {
				health = (char)Item_getHitPoints(&other);
				strongest = i;
			} else {
				GetNpcIbo(&current, strongest);
				if ((char)Item_getHitPoints(&other) == health && allowTargeted && IsTargeting(self, other) &&
					!IsTargeting(self, current)) {
					health = (char)Item_getHitPoints(&other);
					strongest = i;
				} else if ((char)Item_getHitPoints(&other) == health && allowTargeted && IsTargeting(self, other) &&
					IsTargeting(self, current) &&
					Item_getNpcNumber(&other) == NPC(self)->primaryTarget) {
					health = (char)Item_getHitPoints(&other);
					strongest = i;
				}
			}
		}
	}
	return strongest;
}

int far FindLeaderAttacker(objref *self)
{
	int group;
	objref enemy, leader, ally;
	int i, j;

	GetNpcIbo(&leader, CombatGroups.leaders[GetAlignment(self)]);
	group = GetAlignment(&leader);
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&enemy, i);
		if (enemy.valid() && CanVisit(&enemy) &&
			(NPC(&enemy)->primaryTarget == Item_getNpcNumber(&leader) ||
				NPC(&enemy)->secondaryTarget == Item_getNpcNumber(&leader)) &&
			!IsDead(&enemy)) {
			for (j = 0; j < NPC_COUNT; j++) {
				GetNpcIbo(&ally, j);
				if (ally.valid() && IsInMode(&ally, ATTACK_PROTECT) && GetAlignment(&ally) == group &&
					NPC(&ally)->primaryTarget == Item_getNpcNumber(&enemy) &&
					Item_getNpcNumber(self) != j)
					break;
			}
			if (j == NPC_COUNT)
				return Item_getNpcNumber(&enemy);
		}
	}
	return -1;
}

int far FindNearestNpc(objref *self, int x, int y, unsigned char alignments, int modes, char targeted)
{
	int best = 1000, nearest = -1;
	objref other;
	int i, distance;

	/* alignment 4 means any side and mode 10 any mode; both become bit masks */
	if (alignments == 4)
		alignments = 0xf;
	else
		alignments = 1 << alignments;
	if (modes == 10)
		modes = -1;
	else
		modes = 1 << modes;
	for (i = 0; i < NPC_COUNT; i++) {
		if (Item_getNpcNumber(self) == i)
			continue;
		GetNpcIbo(&other, i);
		if (other.valid() && CanVisit(&other) && !IsDead(&other) &&
			(alignments & (1 << GetAlignment(&other))) &&
			(modes & (1 << (unsigned char)Item_getQuality(&other))) &&
			(targeted || !HasTarget(&other))) {
			distance = Item_greatestDeltaToCoords(other, x, y, 0);
			if (distance < best) {
				best = distance;
				nearest = i;
			}
		}
	}
	return nearest;
}

unsigned char far ChooseTarget(NPCRef &self, int distance)
{
	int target = -1;
	int mode;

	if (!WantsPrimary(&self))
		mode = ATTACK_NEAREST;
	else if ((unsigned char)Item_getQuality(&self) == ATTACK_DEFEND
		&& (char)Item_getHitPoints(&self) <= NPC(&self)->strength / 2)
		mode = ATTACK_WEAKEST;
	else
		mode = (unsigned char)Item_getQuality(&self);
	switch (mode) {
	case ATTACK_NEAREST:
		target = ChooseNearestTarget(&self, 0, 1, distance);
		break;
	case ATTACK_WEAKEST:
		target = ChooseWeakestTarget(&self, 0, 1, distance);
		break;
	case ATTACK_STRONGEST:
		target = ChooseStrongestTarget(&self, 0, 1, distance);
		break;
	case ATTACK_BERSERK:
		if (RollChance(3))
			target = ChooseRandomTarget(&self, 0, 1, distance);
		else
			target = ChooseNearestTarget(&self, 0, 1, distance);
		break;
	case ATTACK_DEFEND:
		target = ChooseNearestTarget(&self, 0, 1, distance);
		break;
	case ATTACK_RANDOM:
		target = ChooseRandomTarget(&self, 0, 1, distance);
		break;
	}
	if (target != -1) {
		if (WantsPrimary(&self)) {
			if (NPC(&self)->secondaryTarget != target)
				Npc_setTarget(&self, target, 1);
			else
				return 0;
		} else {
			if (NPC(&self)->primaryTarget != target)
				Npc_setTarget(&self, target, 0);
			else
				return 0;
		}
		return 1;
	}
	return 0;
}

int far FindIdleProtector(objref *self)
{
	int i;
	objref member;
	int group = GetAlignment(self);

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo(&member, i);
		if (member.valid() && CanVisit(&member) && GetAlignment(&member) == group &&
			IsInMode(&member, ATTACK_PROTECT) && !HasTarget(&member) && Item_getNpcNumber(self) != i)
			return i;
	}
	return -1;
}

void far ReassignProtector(objref *self)
{
	NPCRef protector;
	objref member;

	GetNpcIbo(&protector, FindIdleProtector(self));
	if (protector.valid()) {
		GetNpcIbo(&member, FindLeaderAttacker(self));
		if (member.valid()) {
			Npc_setTarget(&protector, Item_getNpcNumber(&member), 1);
			Npc_setTarget(&protector, -1, 0);
		}
	}
}

void far ProtectLeader(NPCRef *self)
{
	NPC(self)->primaryTarget = -1;
	Npc_setTarget(self, FindLeaderAttacker(self), 1);
}

void far ChooseNearbyTarget(objref *self)
{
	ChooseTarget(*self, 12);
}
