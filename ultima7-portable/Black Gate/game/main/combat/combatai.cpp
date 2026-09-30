/* Black Gate U7.EXE, overlay segment 219 (file offsets 0x05b530 to 0x05fa58, 17704 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

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

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)

/* a weapon number, -1 for none */
struct WeaponRef {
	int16_t index;
	WeaponRef() {}
	WeaponRef(int16_t n) { index = n; }
	operator int16_t() { return index; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	uint8_t none() { return index == -1; }
	uint8_t unset() { return index == 0; }
};

uint8_t PartyMissileFlags[8];

#define NPC(p) GetNpcBufferForIbo(p)
#define NPCNUM(p) (uint16_t)Item_getNpcNumber(p)
#define IN_PARTY(n) ((uint8_t) (((n)->status & NPC_IN_PARTY) != 0))
#define ALIGNMENT(n) ((uint8_t) (((n)->status & 0x18) >> 3))
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

const int8_t TRUE = 1;
const int8_t FALSE = 0;

/* enough move points banked to attack */
inline int8_t CanAffordAttack(objref *p)
{
	return CombatGroups.movePoints[NPCNUM(p)] >= 24 ? TRUE : FALSE;
}

inline void SpendAttackPoints(objref *p)
{
	CombatGroups.movePoints[NPCNUM(p)] -= 24;
}

inline uint8_t HasOppressor(objref *p) { return NPC(p)->oppressor != -1; }

/* the health byte has run out */
#define DEAD(p) ((uint8_t) ((int8_t)Item_getHitPoints(p) <= 0))

inline uint8_t IsOnFirstSchedule(objref *p)
{
	return NPC(p)->currentSchedule == 0;
}

inline int8_t HasFallen(objref *p)
{
	return (int8_t)Item_getHitPoints(p) <= 0;
}

/* an NPC number field in use */
inline uint8_t IsAssigned(int16_t n)
{
	return n != -1;
}

inline int8_t IsSame(int16_t a, int16_t b)
{
	return a == b ? TRUE : FALSE;
}

/* enough move points banked to take a step */
inline uint8_t CanAffordStep(objref *p)
{
	return CombatGroups.movePoints[NPCNUM(p)] >= 16;
}

inline void SpendStepPoints(objref *p)
{
	CombatGroups.movePoints[NPCNUM(p)] -= 16;
}

/* iVr holds a map spot to head for, -1 when none is set */
inline uint8_t HasSpot(objref *p)
{
	return NPC(p)->iVr[0] != -1;
}

inline int8_t HasSecondSpot(objref *p)
{
	return NPC(p)->sVr[0] != -1;
}

/* the height of a type-and-frame word's type */
#define HEIGHT(typeFrame) (gItemTypeInfo[(typeFrame) & 0x3ff].height)
/* the step was taken and nothing blocked it */
#define STEPPED(c) (((c) & 0x8000) && !((c) & 0x2000))

/* the next map column or row in direction d */
inline int16_t GetStepX(int16_t x, int8_t d)
{
	return Coord(x + DirDeltaX[d]);
}

inline int16_t GetStepY(int16_t y, int8_t d)
{
	return Coord(y + DirDeltaY[d]);
}

inline int8_t CheckPath(ItemId item, ItemId target)
{
	return HasLineOfFire(item, target);
}

inline int8_t CheckPath(ItemId item, CellCoord x, CellCoord y, int16_t z)
{
	return HasLineOfFireToCoords(item, x, y, z);
}

inline uint8_t MakeItemRef(objref *item, TypeFrame typeFrame)
{
	return CreateItem(item, typeFrame);
}

inline uint8_t MakeItemRef(objref *item, TypeFrame typeFrame, CellCoord x, CellCoord y, int16_t z)
{
	return CreateItem(item, typeFrame, x, y, z);
}

inline uint8_t CanPlace(CellCoord x, CellCoord y, int16_t z, ItemId object)
{
	return CanItemMoveTo(x, y, z, object);
}

inline uint8_t CanPlace(CellCoord x, CellCoord y, int16_t z, TypeFrame &typeFrame)
{
	return CanTypeMoveTo(x, y, z, typeFrame);
}

inline uint8_t CanPlace(CellCoord x, CellCoord y, int16_t z, TypeFrame &&typeFrame)
{
	return CanPlace(x, y, z, typeFrame);
}

/* by direction to the threat: the three directions away from it, repeated so any start reads three */
uint8_t FleeDirections[8][5] = {
	{ 3, 4, 5, 3, 4 }, { 4, 5, 6, 4, 5 }, { 5, 6, 7, 5, 6 }, { 6, 7, 0, 6, 7 },
	{ 7, 0, 1, 7, 0 }, { 0, 1, 2, 0, 1 }, { 1, 2, 3, 1, 2 }, { 2, 3, 4, 2, 3 }
};
char ProtectChance[] = { 100, 75, 50, 25 };
char CallForHelpChance[] = { 100, 85, 70, 55 };

inline uint8_t IsDying(int8_t hp) { return hp <= 0; }
inline uint8_t IsNpcInMode(NPCRef &npc, uint8_t mode)
{
	return (uint8_t)Item_getQuality(&npc) == mode;
}
inline uint8_t IsNpcInMode(NPCRef &&npc, uint8_t mode) { return IsNpcInMode(npc, mode); }
inline int16_t GetPrimaryTarget(NPCRef &npc) { return NPC(&npc)->primaryTarget; }
inline int16_t GetPrimaryTarget(NPCRef &&npc) { return GetPrimaryTarget(npc); }
inline uint8_t HasLeader(uint8_t side) { return CombatGroups.leaders[side] != -1; }

/* an NPC was hit */
extern "C" void HandleNpcHit(objref attacker, objref victim, int8_t hp)
{
	uint8_t protecting;
	int16_t target;
	uint8_t alive, staying, survived;
	int16_t side;

	alive = !IsDying(hp);
	protecting = IsNpcInMode(victim, ATTACK_PROTECT);
	staying = !IsNpcInMode(victim, ATTACK_FLEE);
	target = GetPrimaryTarget(victim);
	side = ALIGNMENT(NPC(&victim));
	if (RollChance(3) && (int8_t)Item_getHitPoints(&victim) != hp)
		SpriteManager_barkOnItem(&gSpriteManager, victim,
			GetGameText(1, GenerateRandomIntegerInRange(4) + 41), 1, 15, 0);
	if (!attacker.valid() || !attacker.isNpc()) {
		if (NPC(&victim)->workType == WORK_COMBAT)
			survived = RespondToAttack(victim, -1, hp);
	} else {
		survived = RespondToAttack(victim, NPCNUM(&attacker), hp);
		if (alive && IsDying((int8_t)Item_getHitPoints(&victim)))
			SpriteManager_barkOnItem(&gSpriteManager, victim, GetGameText(1, 47), 5, 15, 0);
		if (ALIGNMENT(NPC(&attacker)) != side) {
			if (IsSentient(victim) && IsDying((int8_t)Item_getHitPoints(&victim)) && !IsDead(&victim)
				&& !HasLeader(side) && GenerateRandomIntegerInRange(100) < ProtectChance[side])
				RallyProtectors(victim);
			else if (IsSentient(victim) && GenerateRandomIntegerInRange(100) < CallForHelpChance[side])
				CallForHelp(victim);
		}
	}
	if (IsNpcUnconscious(&victim) && !survived)
		RemoveFromCombat(victim);
}

int16_t GetNpcWeapon(objref *npc)
{
	objref item;
	WeaponRef weapon;

	item = GetItemInSlot(*npc, 1);
	if (item.valid()) {
		weapon = WeaponLookup.get(GetItemType(item));
		if (weapon.unset()) {
			weapon = WeaponLookup.get(GetItemType(*npc));
			if (weapon.unset())
				weapon = -1;
		}
	} else {
		weapon = WeaponLookup.get(GetItemType(*npc));
		if (weapon.unset())
			weapon = -1;
	}
	return weapon;
}

uint16_t GetRangeWithWeapon(NPCRef &npc, int16_t *weapon)
{
	return GetWeaponRange(npc, *weapon);
}

uint16_t GetAttackRange(NPCRef *npc)
{
	objref item;
	uint16_t result;

	item = GetItemInSlot(*npc, 1);
	if (item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */ && (uint8_t)Item_isAvatar(npc))
		result = 9;
	else
		result = GetWeaponRange(*npc, GetNpcWeapon(npc));
	return result;
}

inline uint8_t IsFleeing(objref *npc)
{
	return (uint8_t)Item_getQuality(npc) == ATTACK_FLEE;
}

int16_t GetCombatTarget(objref *npc)
{
	if (IsFleeing(npc))
		return NPC(npc)->oppressor;
	if ((uint8_t) (NPC(npc)->typeFlagsHigh & 1))
		return NPC(npc)->primaryTarget;
	return NPC(npc)->secondaryTarget;
}

void CallForHelp(NPCRef &self)
{
	int16_t i;
	int16_t count;
	NPCRef other;

	for (i = 0, count = 0; i < NPC_COUNT && count < 2; i++) {
		GetNpcIbo(&other, i);
		if (other.valid() && CanVisit(&other) && !IsNpcUnconscious(&other)
			&& GetAlignment(&other) == GetAlignment(&self) && !HasSecondary(&other)
			&& NPC(&other)->primaryTarget != NPC(&self)->oppressor && IsSentient(other)
			&& !IsInMode(&other, ATTACK_FLEE) && !IsInMode(&other, ATTACK_PROTECT)
			&& (!IsInParty(&other) || IsAvatarInCombat())
			&& (HasLineOfFire(self, other) || Item_greatestDeltaToItem(self, other) <= 25)
			&& (!IsInMode(&other, ATTACK_FLANK) || !HasSpot(&other))) {
			if (!HasTarget(&other))
				NPC(&other)->typeFlagsHigh = NPC(&other)->typeFlagsHigh & ~1;
			Npc_setTarget(&other, NPC(&self)->oppressor, 0);
			count++;
		}
	}
}

/* drop an NPC from combat: clear every target that points at it */
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
	if (work != WORK_COMBAT && CUR_SCHED(&self).state == 0 && !active) {
		Schedule_getCoord(&ScheduleTable, NPCNUM(&self), SchedulePeriod, &x, &y);
		if (GetDistance(Item_getX(AvatarRef), Item_getY(AvatarRef),
			Item_getZ(&AvatarRef), x, y, 0) >= 21)
			Item_move(&self, CellCoord(x), CellCoord(y));
		else if (!PlaceNpcNearAvatar(&self, x, y, 1)) {
			Npc_setSchedule(&self, WORK_WANDER);
			CUR_SCHED(&self).state = 1;
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

void Npc_setTarget(NPCRef *npc, int16_t target, int8_t primary)
{
	objref victim;
	NPCRef attacker;
	int16_t monster;

	if (primary && (uint8_t)Item_isAvatar(npc))
		RemoveFromCombat(*npc);
	if (!IsInCombat(npc) && target != -1) {
		Npc_setSchedule(npc, WORK_COMBAT);
		CUR_SCHED(npc).state = 1;
	}
	GetNpcIbo(&victim, target);
	if (victim.valid() && (uint8_t) (Item_getQualityFlags(&victim) & QUALITY_INVISIBLE)) {
		monster = MonsterLookup.get(GetItemType(*npc));
		if (!(uint8_t) MonsterRecords.get(monster)->seeInvisible) {
			NPC(npc)->typeFlagsHigh = NPC(npc)->typeFlagsHigh | 1;
			return;
		}
	}
	if (primary)
		NPC(npc)->primaryTarget = target;
	else
		NPC(npc)->secondaryTarget = target;
	if (CombatGroups.isProtectee(target)) {
		GetNpcIbo(&attacker, FindNearestNpc(npc, Item_getX(*npc), Item_getY(*npc),
			ALIGNMENT(NPC(&victim)), 4, 0));
		if (attacker.valid() && NPC(&attacker)->workType == WORK_COMBAT) {
			Npc_setTarget(&attacker, NPCNUM(npc), 1);
			NPC(&attacker)->typeFlagsHigh = NPC(&attacker)->typeFlagsHigh | 1;
		}
	}
}

/* Picks a weapon or spell for npc and says whether its target is in reach. */
uint8_t PrepareAttack(NPCRef *npc)
{
	objref target;
	WeaponRef weapon;
	int16_t range;
	int16_t damage;
	objref item;
	uint8_t casting = 0;
	uint8_t partyMember;
	int8_t rearm = 0;
	int8_t unconscious;
	int8_t ranged = 0;
	int16_t type;
	int16_t ammoCount;
	struct WeaponRecord details;
	MonsterRecord monster;

	partyMember = IN_PARTY(NPC(npc));
	item = GetItemInSlot(*npc, 1);
	GetNpcIbo(&target, GetCombatTarget(npc));
	if ((uint8_t)Item_isAvatar(npc) && item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */) {
		if (CanCastSpell(*npc, (uint8_t)GetSpellbookBookmark(&NPCRef(item.off)), 1, 1)) {
			casting = 1;
			range = 9;
		} else {
			SelectWeapon(*npc, 0, 0);
			weapon = GetNpcWeapon(npc);
		}
	} else
		weapon = GetNpcWeapon(npc);
	if (!casting) {
		ammoCount = CountWeaponAmmo(*npc, weapon);
		unconscious = IsNpcUnconscious(&target);
		if (ammoCount && !partyMember && (unconscious || RollChance(5))) {
			if (weapon.none()) {
				type = GetMonsterNumber(*npc);
				MonsterRecords.read(type, &monster);
				damage = monster.damage;
			} else {
				WeaponRecords.read(weapon, &details);
				damage = details.damage;
			}
			if (unconscious) {
				if (damage == 0)
					rearm = 1;
				else if (RollChance(3))
					rearm = 1;
			} else if (!weapon.none()) {
				if (WeaponRecords.get(weapon)->range >= 6)
					ranged = 1;
				else if (damage == 0)
					rearm = 1;
			}
		}
		if (!ammoCount || rearm || ranged) {
			SelectWeapon(*npc, !partyMember, ranged ? 6 : 0);
			weapon = GetNpcWeapon(npc);
			ammoCount = CountWeaponAmmo(*npc, weapon);
		}
		if (ammoCount == 1 && WeaponRecords.get(weapon)->ammo == -3 && item.valid()
			&& TYPE(ITEM(item.off)) != 782 /* flaming oil */ && TYPE(ITEM(item.off)) != 565 /* starburst */)
			range = WeaponRecords.get(weapon)->range;
		else
			range = GetAttackRange(npc);
	}
	if (GetMissileDistance(*npc, target) <= range && HasLineOfFire(*npc, target))
		return 1;
	return 0;
}

uint8_t IsTargetInReach(NPCRef *attacker, objref target)
{
	if ((uint16_t)(GetMissileDistance(*attacker, target)) <= GetAttackRange(attacker)
		&& HasLineOfFire(*attacker, target))
		return 1;
	return 0;
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

uint8_t FindSpotNextToItem(objref *self, objref item, Coord *outX, Coord *outY, int16_t *outZ)
{
	int16_t baseX, baseY, i, spanX, spanY, z, baseZ;
	int16_t x, y;
	char zOffsets[5] = { 0, 1, -1, 2, -2 };

	spanX = GetFootprintX(ITEM(item.off)->asTypeFrame()) + 1;
	spanY = GetFootprintY(ITEM(item.off)->asTypeFrame()) + 1;
	baseX = Item_getX(item);
	baseY = Item_getY(item);
	baseZ = Item_getZ(&item);
	for (i = 0; i < 5; i++) {
		z = baseZ + zOffsets[i];
		if (z > 15 || z < 0)
			continue;
		for (x = 0; x <= spanX + 1; x++)
			for (y = 0; y <= spanY + 1; y++)
				if (CanPlace(Coord(Coord(baseX - spanX) + x), Coord(Coord(baseY - spanY) + y), z, item)) {
					*outX = Coord(Coord(baseX - spanX) + x);
					*outY = Coord(Coord(baseY - spanY) + y);
					*outZ = z;
					return 1;
				}
	}
	return 0;
}

uint8_t StepToward(objref *npc, Coord targetX, Coord targetY, int16_t targetZ, int16_t stride)
{
	Coord x, y;
	int16_t dx, dy, sx, sy;
	int16_t i;
	int16_t dz = 0;
	int16_t movement;
	uint16_t typeFrame;
	uint16_t collision;
	int8_t dir;
	int8_t state = 8;         /* 8 level, 9 climbing, 10 descending */
	int8_t turn;
	uint8_t moved = 0;
	int8_t unused = 0;
	int16_t z;

	x = Item_getX(*npc);
	y = Item_getY(*npc);
	z = Item_getZ(npc);
	typeFrame = ITEM(npc->off)->typeFrame;
	movement = NPC(npc)->typeFlags;
	dx = GetDelta(targetX, x);
	dy = GetDelta(targetY, y);
	sx = GetSign(dx);
	sy = GetSign(dy);
	dir = DirectionBySign[sx + 1][sy + 1];
	Item_detach(npc);
	if (dir == -1)
		collision = 0;
	else {
		SetNPCFacing(*npc, dir);
		collision = CheckMove(x, y, z, typeFrame, dir, stride, movement);
		if (STEPPED(collision))
			moved = 1;
		else
			for (i = -1; i <= 1; i += 2) {
				turn = (dir + i) & 7;
				SetNPCFacing(*npc, turn);
				collision = CheckMove(x, y, z, typeFrame, turn, stride, movement);
				if (STEPPED(collision)) {
					dir = turn;
					moved = 1;
					break;
				}
			}
	}
	if ((uint8_t) (NPC(npc)->typeFlags & 0x10) && (int8_t) collision == 0) {
		if (!moved) {
			if (CanFlyAt(x, y, z + 1, typeFrame) && z + HEIGHT(typeFrame) < 15)
				state = 9;
			else {
				PlaceItem(npc, x, y, z);
				return 0;
			}
		}
		if (state == 8) {
			if (z < targetZ && CanFlyAt(GetStepX(x, dir), GetStepY(y, dir), z + 1, typeFrame)
				&& z + HEIGHT(typeFrame) < 15)
				state = 9;
			else if (z > targetZ && CanFlyAt(GetStepX(x, dir), GetStepY(y, dir), z - 1, typeFrame))
				state = 10;
		}
		if (state == 9)
			dz = 1;
		else if (state == 10)
			dz = -1;
	}
	PlaceItem(npc, x, y, z);
	if (!moved && (!(uint8_t) (NPC(npc)->typeFlags & 0x10) || state == 8))
		return 0;
	if ((uint8_t) (NPC(npc)->typeFlags & 0x10)) {
		if (moved)
			StepItem(*npc, dir, (int8_t) collision ? (int8_t) collision : dz, 1);
		else
			StepItem(*npc, 8, dz, 1);
	} else if (!(uint8_t) (NPC(npc)->typeFlags & 0x10))
		StepItem(*npc, dir, (int8_t) collision, stride);
	return 1;
}

uint8_t ApproachTarget(NPCRef *npc)
{
	NPCRef target;
	Coord x, y;
	int16_t stride;
	int16_t unusedFlags;
	uint8_t moved;
	uint8_t pathed;
	int16_t z;

	if (!CanMove(npc))
		return 0;
	unusedFlags = NPC(npc)->typeFlags;
	if (IN_PARTY(NPC(npc)) && PartyMissileFlags[GetPartyIndex(*npc)]) {
		if (CanAffordStep(npc))
			SpendStepPoints(npc);
		else
			return 1;
	} else {
		stride = GetMoveStride(npc);
		if (!CanAffordStep(npc))
			return 1;
		if (HasSpot(npc)) {
			x = NPC(npc)->iVr[0];
			y = NPC(npc)->iVr[1];
			z = Item_getZ(npc);
			moved = 0;
			if ((int8_t) (Npc_getIntelligence(npc) >= 15))
				moved = PathToSpot(npc, x, y, z, 60);
			if (!moved)
				moved = StepToward(npc, x, y, z, stride);
			if (!moved || GetDistance(Item_getX(*npc), Item_getY(*npc), Coord(NPC(npc)->iVr[0]),
				Coord(NPC(npc)->iVr[1])) <= (uint8_t) (NPC(npc)->typeFlags & 7)) {
				if (HasSecondSpot(npc))
					UseSecondSpot(npc);
				else {
					NPC(npc)->iVr[0] = -1;
					if (!IsInMode(npc, ATTACK_PROTECT))
						SetAttackMode(*npc, ATTACK_DEFEND);
				}
				return 0;
			}
		} else {
			GetNpcIbo(&target, GetCombatTarget(npc));
			if (CanTeleport(npc) && Item_greatestDeltaToItem(*npc, target) >= 15 && CanAffordAttack(npc)
				&& RollChance(5)
				&& TeleportInCombat(npc, 1)) {
				SpendAttackPoints(npc);
				return 1;
			}
			x = Item_getX(target);
			y = Item_getY(target);
			z = Item_getZ(&target);
			if (!(uint8_t) (NPC(npc)->typeFlags & 0x10) && (int8_t) (Npc_getIntelligence(npc) >= 15)) {
				if (!PathNextToItem(npc, target, 60)) {
					if (!StepToward(npc, x, y, z, stride))
						return 0;
				}
			} else {
				pathed = 0;
				if (!(uint8_t) (NPC(npc)->typeFlags & 0x10) && stride == 1)
					pathed = PathNextToItem(npc, target, 25);
				if (!pathed) {
					if (!StepToward(npc, x, y, z, stride))
						return 0;
				}
			}
		}
		CombatGroups.someoneActed = 1;
		SpendStepPoints(npc);
	}
	return 1;
}

int8_t IsRangedAttack(objref *npc, int16_t weaponNumber, objref target)
{
	WeaponRef weapon = weaponNumber;
	int16_t dist;
	int16_t mode;

	if (weapon.none())
		return 0;
	mode = (uint8_t)weapon->uses;
	if (mode == 3)
		return 1;
	if (mode == 0)
		return 0;
	dist = Item_greatestDeltaToItem(*npc, target);
	return weapon->range < (uint16_t)dist;
}

int16_t InvisibleTypes[] = { 445, 446, 154, 317, 299, 519, 504, 511, 354, -1 };

int8_t CanTurnInvisible(objref *p)
{
	int16_t type = TYPE(ITEM(p->off));
	int16_t i;

	for (i = 0; InvisibleTypes[i] != -1; i++)
		if (InvisibleTypes[i] == type)
			return 1;
	return 0;
}

int16_t TeleportingTypes[] = { 445, 446, 154, 317, 299, 382, 519, 534, 354, -1 };

int8_t CanTeleport(objref *p)
{
	int16_t type = TYPE(ITEM(p->off));
	int16_t i;

	for (i = 0; TeleportingTypes[i] != -1; i++)
		if (TeleportingTypes[i] == type)
			return 1;
	return 0;
}

int16_t SummoningTypes[] = { 445, 446, 154, 519, 354, -1 };

int8_t CanSummon(objref *p)
{
	int16_t type = TYPE(ITEM(p->off));
	int16_t i;

	for (i = 0; SummoningTypes[i] != -1; i++)
		if (SummoningTypes[i] == type)
			return 1;
	return 0;
}

void TakeCombatTurn(NPCRef *npc)
{
	uint16_t reach;     /* the target's own attack range */
	NPCRef target;
	uint16_t range;
	objref weapon = GetNpcWeapon(npc);
	uint8_t atRange;
	int16_t dist;
	int16_t skill;
	int16_t ammoCount = 100;
	int16_t z, dz, newZ, height, targetMiddle;
	int8_t casting = 0;
	int8_t fleeing;
	objref item;

	skill = Npc_getIntelligence(npc);
	GetNpcIbo(&target, GetCombatTarget(npc));
	fleeing = (uint8_t)Item_getQuality(npc) == ATTACK_FLEE;
	if (IN_PARTY(NPC(npc)) && fleeing && ALIGNMENT(NPC(npc)) == ALIGNMENT(NPC(&target))) {
		NPC(npc)->oppressor = -1;
		SpendAttackPoints(npc);
		return;
	}
	item = GetItemInSlot(*npc, 1);
	if (item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */ && (uint8_t)Item_isAvatar(npc)) {
		casting = 1;
	} else {
		ammoCount = CountWeaponAmmo(*npc, weapon);
	}
	if (ammoCount == 1 && WeaponRecords.get(weapon)->ammo == -3)
		range = WeaponRecords.get(weapon)->range;
	else
		range = GetAttackRange(npc);
	reach = GetAttackRange(&target);
	dist = Item_greatestDeltaToItem(*npc, target);
	if (IsNpcUnconscious(&target)) {
		if (dist > 8 && RollChance(2) && ApproachTarget(npc))
			return;
	} else {
		if (CanTeleport(npc) && CanAffordAttack(npc) && GenerateRandomIntegerInRange(45) < skill
			&& ((uint16_t)dist <= reach && (range > reach || RollChance(2))
			|| RollChance(3) && GenerateRandomIntegerInRange(60) < skill)
				&& TeleportInCombat(npc, fleeing)) {
			CombatGroups.someoneActed = 1;
			SpendAttackPoints(npc);
			return;
		}
		if (range > reach && ((uint16_t)dist <= reach || (uint16_t)(dist + 1) <= range && RollChance(2))
			&& (CombatGroups.movePoints[NPCNUM(npc)] >= CombatGroups.movePoints[NPCNUM(&target)] + 16 || RollChance(2))
			&& (int8_t) (Npc_getIntelligence(npc) >= 18) && FleeStep(npc, fleeing, 1, 0))
			return;
	}
	if (CanAffordAttack(npc) == 0)
		return;
	if (!(uint8_t) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE) && CanTurnInvisible(npc)
		&& GenerateRandomIntegerInRange(375) < skill) {
		SpriteManager_playSpriteForItem(&gSpriteManager, *npc, 0, 0, 0, 0, 1036, 0, -1, 5);
		PlaySoundAtItem(44, *npc);
		SpendAttackPoints(npc);
		Item_setInvisible(npc);
		CombatGroups.someoneActed = 1;
		return;
	}
	if (CanSummon(npc) && GenerateRandomIntegerInRange(600) < skill && SummonMonsters(npc)) {
		SpendAttackPoints(npc);
		CombatGroups.someoneActed = 1;
		return;
	}
	if ((uint8_t) (NPC(npc)->typeFlags & 0x10) && RollChance(4)) {
		z = Item_getZ(npc);
		if (RollChance(3))
			dz = RollChance(2) ? 1 : -1;
		else {
			height = gItemTypeInfo[TYPE(ITEM(npc->off))].height;
			targetMiddle = Item_getZ(&target) + gItemTypeInfo[TYPE(ITEM(target.off))].height - height / 2;
			if (z < targetMiddle)
				dz = 1;
			else
				dz = -1;
		}
		newZ = z + dz;
		if (newZ + height <= 15 && newZ >= 0) {
			if (CanFlyAt(Item_getX(*npc), Item_getY(*npc), newZ, TYPE(ITEM(npc->off)))) {
				StepItem(*npc, 8, dz, 1);
				SpendAttackPoints(npc);
				CombatGroups.someoneActed = 1;
				return;
			}
		}
	}
	if (casting || IsRangedAttack(npc, weapon, target))
		atRange = 1;
	else
		atRange = 0;
	if (CombatGroups.isNPCMarked(NPCNUM(npc)) && atRange) {
		SpendAttackPoints(npc);
		return;
	}
	if (!atRange) {
		CombatGroups.markNPC(NPCNUM(&target));
		if ((NPC(&target)->workType == WORK_COMBAT || !IN_PARTY(NPC(&target)))
			&& (NPCNUM(&target) != 150 || NPC(&target)->workType != WORK_SLEEP)) {
			if ((uint16_t)NPC(&target)->primaryTarget == NPCNUM(npc))
				NPC(&target)->typeFlagsHigh = NPC(&target)->typeFlagsHigh | 1;
			else if ((uint16_t)NPC(&target)->secondaryTarget == NPCNUM(npc))
				NPC(&target)->typeFlagsHigh = NPC(&target)->typeFlagsHigh & ~1;
			else {
				if (NPC(&target)->workType == WORK_SLEEP)
					WakeUpNpc(&target);
				if (ALIGNMENT(NPC(&target)) != ALIGNMENT(NPC(npc))) {
					if (NPC(&target)->workType != WORK_COMBAT && ALIGNMENT(NPC(&target)) == 0
						&& NPCNUM(&target) < 256 && IN_PARTY(NPC(npc)))
						RunUsable(1, *npc, 0x63a);
					NPC(&target)->typeFlagsHigh = NPC(&target)->typeFlagsHigh & ~1;
					Npc_setTarget(&target, NPCNUM(npc), 0);
				}
			}
		}
	}
	CombatGroups.someoneActed = 1;
	SpendAttackPoints(npc);
	if (casting) {
		Npc_setItemTarget(&AvatarRef, target);
		TryToCastSpell(AvatarRef, (uint8_t)GetSpellbookBookmark(&NPCRef(item.off)), 1, 1, 4);
	} else
		AttackItemWithWeapon(*npc, target, weapon);
	if (RollChance(15))
		SpriteManager_barkOnItem(&gSpriteManager, *npc,
			GetGameText(1, GenerateRandomIntegerInRange(7) + 83), 0, 15, 0);
}

uint8_t RespondToAttack(NPCRef &self, int16_t attacker, int8_t hp)
{
	objref secondary;
	objref primary;
	NPCRef target;
	objref other;
	int8_t result = 0;
	uint8_t down;
	int8_t calm, ally;
	int16_t align, targetAlign, health;
	uint16_t dist;
	int16_t mode;
	int16_t partyIndex;
	int16_t monster;
	int16_t type;
	int16_t i;

	GetNpcIbo(&target, attacker);
	align = ALIGNMENT(NPC(&self));
	health = (int8_t)Item_getHitPoints(&self);
	targetAlign = ALIGNMENT(NPC(&target));
	down = IsNpcUnconscious(&self);
	type = TYPE(ITEM(self.off));
	partyIndex = GetPartyIndex(self);
	if (IN_PARTY(NPC(&self)) && PartyMissileFlags[partyIndex])
		PartyMissileFlags[partyIndex] = 0;
	if (down && (uint8_t)Item_isAvatar(&self) && NPC(&self)->workType)
		BeginCombat();
	if (attacker != -1) {
		if (IN_PARTY(NPC(&target)) && (align == 2 || align == 3) && !CombatGroups.partyAttacked) {
			CombatGroups.partyAttacked = 1;
			CombatGroups.partyHealth = CombatGroups.sumGroupHealth(1);
			CombatGroups.enemyHealth = CombatGroups.sumGroupHealth(2) + CombatGroups.sumGroupHealth(3) + (hp - health);
		}
		if (!IsInCombat(&self)) {
			if (IN_PARTY(NPC(&self)) || NPCNUM(&self) == 150 && NPC(&self)->workType == WORK_SLEEP)
				return result;
			if (RollChance(3))
				BarkLine(&self, GenerateRandomIntegerInRange(2) + 45, 2);
			Npc_setSchedule(&self, WORK_COMBAT);
			CUR_SCHED(&self).state = 1;
			if (IsInMode(&self, ATTACK_FLANK)) {
				SetAttackMode(self, ATTACK_NEAREST);
				result = 1;
			}
			if (align == 0 && NPCNUM(&self) < 256 && IN_PARTY(NPC(&target)))
				RunUsable(1, self, 0x63a);
		} else if (IsInMode(&self, ATTACK_FLANK)
			&& (!IsAssigned(NPC(&self)->iVr[0]) || GenerateRandomIntegerInRange(100) < 50)) {
			NPC(&self)->iVr[0] = -1;
			SetAttackMode(self, ATTACK_DEFEND);
			result = 1;
		}
	}
	if (NPC(&self)->workType == WORK_COMBAT) {
		if (!down && attacker != -1 && IsInMode(&self, ATTACK_FLEE) && align != targetAlign) {
			monster = GetMonsterNumber(self);
			if (!FleeStep(&self, 1, 0, 1)
				&& (!(uint8_t) (Item_getQualityFlags(&target) & QUALITY_INVISIBLE) ||
					(uint8_t) MonsterRecords.get(monster)->seeInvisible)) {
				dist = GetMissileDistance(self, target);
				if (GetAttackRange(&target) >= dist && GetAttackRange(&self) < dist) {
					if (RollChance(4)) {
						mode = ATTACK_BERSERK;
						BarkLine(&self, GenerateRandomIntegerInRange(3) + 140, 3);
					} else {
						mode = ATTACK_NEAREST;
						BarkLine(&self, GenerateRandomIntegerInRange(3) + 143, 3);
					}
					SetAttackMode(self, mode);
					result = 1;
					Npc_setTarget(&self, attacker, 1);
				}
			}
		} else if (!(uint8_t)Item_isAvatar(&self) && !IsInMode(&self, ATTACK_BERSERK)
			&& !IsInMode(&self, ATTACK_FLEE) &&
			NPC(&self)->strength / 4 >= health) {
			SetAttackMode(self, ATTACK_FLEE);
			result = 1;
		}
	}
	NPC(&self)->oppressor = attacker;
	if (down)
		return result;
	if (align == targetAlign)
		return result;
	if (!IN_PARTY(NPC(&self)) && !InDungeon && type != 519 && type != 317 && type != 299 && RollChance(3)) {
		if (NPCNUM(&self) < 256) {
			if (ALIGNMENT(NPC(&self)) == 0) {
				BarkLine(&self, GenerateRandomIntegerInRange(5) + 105, 3);
				ally = 0;
				calm = 1;
				for (i = 0; i < NPC_COUNT; i++) {
					GetNpcIbo(&other, i);
					if (other.valid() && CanVisit(&other)) {
						if (TYPE(ITEM(other.off)) == 946 /* guard */ && NPC(&other)->workType == WORK_COMBAT
							&& CUR_SCHED(&other).kind == '0')
							calm = 0;
						if (i < 256 && ALIGNMENT(NPC(&other)) == 0 && other != self) {
							ally = 1;
							/* one townsperson is enough; only guards matter now */
							if (i < 255)
								i = 255;
						}
					}
				}
				if (ally)
					CombatGroups.callGuards(calm);
			}
		} else if (TYPE(ITEM(self.off)) == 946 /* guard */ && RollChance(3)) {
			BarkLine(&self, GenerateRandomIntegerInRange(5) + 105, 3);
			CombatGroups.callGuards(1);
		}
	}
	GetNpcIbo(&secondary, NPC(&self)->secondaryTarget);
	if (!IsTargeting(&self, target)
		&& (!IsAssigned(NPC(&self)->secondaryTarget) || !IsTargetInReach(&self, secondary) || HasFallen(&secondary) ||
			IsInMode(&secondary, ATTACK_FLEE))) {
		Npc_setTarget(&self, attacker, 0);
		secondary = target;
	}
	GetNpcIbo(&primary, NPC(&self)->primaryTarget);
	if (IsSame(NPC(&self)->oppressor, NPC(&self)->primaryTarget) && !(uint8_t) (NPC(&self)->typeFlagsHigh & 1)
		&& (!PrepareAttack(&self) || HasFallen(&secondary) || IsInMode(&secondary, ATTACK_FLEE)))
		NPC(&self)->typeFlagsHigh = NPC(&self)->typeFlagsHigh | 1;
	else if (IsSame(NPC(&self)->oppressor, NPC(&self)->secondaryTarget) &&
		(uint8_t) (NPC(&self)->typeFlagsHigh & 1) && !IsInMode(&self, ATTACK_PROTECT)
		&& (!IsAssigned(NPC(&self)->primaryTarget) || !PrepareAttack(&self) || HasFallen(&primary)
			|| IsInMode(&primary, ATTACK_FLEE)))
		NPC(&self)->typeFlagsHigh = NPC(&self)->typeFlagsHigh & ~1;
	return result;
}

uint8_t IsSentient(NPCRef &npc)
{
	int16_t monster = MonsterLookup.get(GetItemType(npc));

	return (uint8_t) MonsterRecords.get(monster)->intelligence >= 6;
}

uint8_t FleeStep(objref *npc, int8_t panic, int8_t spend, int8_t mayTeleport)
{
	AreaSearch inventory, contents;
	objref target;
	int16_t i;
	int16_t first;
	uint8_t dir;
	uint8_t step;
	uint16_t collision;
	int16_t stride;
	objref blood;
	objref item;
	objref next;

	if (!CanMove(npc))
		return 0;
	if (spend && CombatGroups.movePoints[NPCNUM(npc)] < (panic == 1 ? 12 : 16))
		return 1;
	stride = GetMoveStride(npc);
	if (stride == 1 && panic && spend && mayTeleport && CanTeleport(npc) && CanAffordAttack(npc)
		&& GenerateRandomIntegerInRange(100) < Npc_getIntelligence(npc) && TeleportInCombat(npc, 1)) {
		SpendAttackPoints(npc);
		CombatGroups.someoneActed = 1;
		return 1;
	}
	GetNpcIbo(&target, GetCombatTarget(npc));
	first = GenerateRandomIntegerInRange(3);
	dir = GetDirectionTo(npc, target);
	RemoveTypeFromCollision(*npc);
	for (i = 0; i < 3; i++) {
		step = FleeDirections[dir][first + i];
		collision = CheckMove(Item_getX(*npc), Item_getY(*npc), Item_getZ(npc),
			ITEM(npc->off)->typeFrame, step, stride, NPC(npc)->typeFlags);
		if (STEPPED(collision)
			&& (panic || CheckPath(target, Coord(Item_getX(*npc) + DirDeltaX[step]),
				Coord(Item_getY(*npc) + DirDeltaY[step]), Item_getZ(npc) + (int16_t) (int8_t) collision))) {
			AddTypeToCollision(*npc);
			if (spend) {
				CombatGroups.someoneActed = 1;
				CombatGroups.movePoints[NPCNUM(npc)] -= panic == 1 ? 12 : 16;
				StepItem(*npc, step, (int8_t) collision, stride);
				/* a bleeding NPC in a panic leaves blood and drops things */
				if (panic && (uint8_t) (NPC(npc)->typeFlagsHigh & 4)) {
					if (RollChance(20)) {
						MakeItemRef(&blood, 912 /* blood */, Item_getX(*npc), Item_getY(*npc),
							Item_getZ(npc));
						if (blood.valid()) {
							Item_setFrame(&blood, GenerateRandomIntegerInRange(4));
							Item_setTemporary(&blood);
							if (RollChance(2))
								ITEM(blood.off)->typeFrame |= 0x8000;
						}
					}
					if (RollChance(25)) {
						item.off = 0;
						if (IN_PARTY(NPC(npc))) {
							if (NPC(npc)->defaultAttackMode != ATTACK_FLEE) {
								item = GetItemInSlot(*npc, 1);
								if (!item.valid())
									item = GetItemInSlot(*npc, 2);
								if (!item.valid())
									item = GetItemInSlot(*npc, 8);
								if (!item.valid())
									item = GetItemInSlot(*npc, 9);
							}
						} else {
							FindItemInContainer(&inventory, *npc, 0, -1, 0xff, 0xff);
							while (inventory.current.valid()
								&& TYPE(ITEM(inventory.current.off)) == 377 /* food item */)
								FindItem(&inventory);
							item = objref(inventory.current.off);
						}
						dir = GenerateRandomIntegerInRange(8);
						if (item.valid()
							&& CanPlace(Coord(Item_getX(*npc) + DirDeltaX[dir]),
							Coord(Item_getY(*npc) + DirDeltaY[dir]), Item_getZ(npc),
							ITEM(item.off)->typeFrame)) {
							if ((uint8_t) ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(item.off))))->spell)
								Item_delete(&item);
							else {
								Item_move(&item, Coord(Item_getX(*npc) + DirDeltaX[dir]),
									Coord(Item_getY(*npc) + DirDeltaY[dir]), Item_getZ(npc));
								Item_setOkayToTake(&item);
								FindItemInContainer(&contents, item, 0, -1, 0xff, 0xff);
								while (contents.current.valid()) {
									Item_setOkayToTake(&contents.current);
									if ((uint8_t) ReadyRecords.get(
										ReadyLookup.get(TYPE(ITEM(contents.current.off))))->spell) {
										next = contents.current;
										FindItem(&contents);
										Item_delete(&next);
									} else
										FindItem(&contents);
								}
							}
						}
					}
				}
			}
			return 1;
		}
	}
	AddTypeToCollision(*npc);
	return 0;
}

uint8_t CanMove(objref *p)
{
	int16_t flags = NPC(p)->typeFlags;

	if (flags & 0x20 || flags & 0x10 || flags & 0x40 || flags & 0x80)
		return 1;
	return 0;
}

inline uint8_t IsMultipart(objref *p)
{
	return gItemTypeInfo[TYPE(ITEM(p->off))].strangeMovement;
}

int16_t GetMoveStride(objref *p)
{
	if (IsMultipart(p) && HasEvenCellPlacement(TYPE(ITEM(p->off))))
		return 2;
	if (TYPE(ITEM(p->off)) == 525 /* sea serpent */)
		return 4;
	return 1;
}

/* the direction from one item to another */
uint8_t GetDirectionTo(objref *from, objref to)
{
	return DirectionBySign[GetSign(Item_getX(to) - Item_getX(*from)) + 1]
		[GetSign(Item_getY(to) - Item_getY(*from)) + 1];
}
