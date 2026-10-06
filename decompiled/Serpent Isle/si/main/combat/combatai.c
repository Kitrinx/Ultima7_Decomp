/* Serpent Isle SI.EXE, overlay segment 348 (file offsets 0x0a4270 to 0x0a839e, 16686 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: combatai.c */
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
#include "actqueue.h"
#include "script.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)

/* a weapon number, -1 for none */
struct WeaponRef {
	int index;
	WeaponRef() {}
	WeaponRef(int n) { index = n; }
	operator int() { return index; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	unsigned char none() { return index == -1; }
	unsigned char unset() { return index == 0; }
};

struct MonsterRef {
	int index;
	MonsterRef() {}
	MonsterRef(int n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
	unsigned char none() { return index == 0; }
};

unsigned char PartyMissileFlags[8];

#define NPC(p) GetNpcBufferForIbo(p)
#define NPCNUM(p) (unsigned)Item_getNpcNumber(p)
#define IN_PARTY(n) ((unsigned char) (((n)->status & NPC_IN_PARTY) != 0))
#define ALIGNMENT(n) ((unsigned char) (((n)->status & 0x18) >> 3))
#define CUR_SCHED(p) (NPC(p)->schedules[NPC(p)->currentSchedule])

const char TRUE = 1;
const char FALSE = 0;

/* enough move points banked to attack */
inline char CanAffordAttack(objref *p)
{
	return CombatGroups.movePoints[NPCNUM(p)] >= 24 ? TRUE : FALSE;
}

inline void SpendAttackPoints(objref *p)
{
	CombatGroups.movePoints[NPCNUM(p)] -= 24;
}

inline unsigned char HasOppressor(objref *p) { return NPC(p)->oppressor != -1; }

/* the health byte has run out */
#define DEAD(p) ((unsigned char) ((char)Item_getHitPoints(p) <= 0))

inline unsigned char IsOnFirstSchedule(objref *p)
{
	return NPC(p)->currentSchedule == 0;
}

inline char HasFallen(objref *p)
{
	return (char)Item_getHitPoints(p) <= 0;
}

/* an NPC number field in use */
inline unsigned char IsAssigned(int n)
{
	return n != -1;
}

inline char IsSame(int a, int b)
{
	return a == b ? TRUE : FALSE;
}

/* enough move points banked to take a step */
inline unsigned char CanAffordStep(objref *p)
{
	return CombatGroups.movePoints[NPCNUM(p)] >= 16;
}

inline void SpendStepPoints(objref *p)
{
	CombatGroups.movePoints[NPCNUM(p)] -= 16;
}

/* iVr holds a map spot to head for, -1 when none is set */
inline unsigned char HasSpot(objref *p)
{
	return NPC(p)->iVr[0] != -1;
}

inline char HasSecondSpot(objref *p)
{
	return NPC(p)->sVr[0] != -1;
}

/* the height of a type-and-frame word's type */
#define HEIGHT(typeFrame) (gItemTypeInfo[(typeFrame) & 0x3ff].height)
/* the step was taken and nothing blocked it */
#define STEPPED(c) (((c) & 0x8000) && !((c) & 0x2000))

/* the next map column or row in direction d */
inline int GetStepX(int x, char d)
{
	return Coord(x + DirDeltaX[d]);
}

inline int GetStepY(int y, char d)
{
	return Coord(y + DirDeltaY[d]);
}

inline char CheckPath(ItemId item, ItemId target)
{
	return HasLineOfFire(item, target);
}

inline char CheckPath(ItemId item, CellCoord x, CellCoord y, int z)
{
	return HasLineOfFireToCoords(item, x, y, z);
}

inline unsigned char MakeItemRef(objref *item, TypeFrame typeFrame)
{
	return CreateItem(item, typeFrame);
}

inline unsigned char MakeItemRef(objref *item, TypeFrame typeFrame, CellCoord x, CellCoord y, int z)
{
	return CreateItem(item, typeFrame, x, y, z);
}

inline unsigned char CanPlace(CellCoord x, CellCoord y, int z, ItemId object)
{
	return CanItemMoveTo(x, y, z, object);
}

inline unsigned char CanPlace(CellCoord x, CellCoord y, int z, TypeFrame far &typeFrame)
{
	return CanTypeMoveTo(x, y, z, typeFrame);
}

/* by direction to the threat: the three directions away from it, repeated so any start reads three */
unsigned char FleeDirections[8][5] = {
	{ 3, 4, 5, 3, 4 }, { 4, 5, 6, 4, 5 }, { 5, 6, 7, 5, 6 }, { 6, 7, 0, 6, 7 },
	{ 7, 0, 1, 7, 0 }, { 0, 1, 2, 0, 1 }, { 1, 2, 3, 1, 2 }, { 2, 3, 4, 2, 3 }
};
char ProtectChance[] = { 100, 75, 50, 25 };
char CallForHelpChance[] = { 100, 85, 70, 55 };

inline unsigned char IsDying(char hp) { return hp <= 0; }
inline unsigned char IsNpcInMode(NPCRef &npc, unsigned char mode)
{
	return (unsigned char)Item_getQuality(&npc) == mode;
}
inline int GetPrimaryTarget(NPCRef &npc) { return NPC(&npc)->primaryTarget; }
inline unsigned char HasLeader(unsigned char side) { return CombatGroups.leaders[side] != -1; }

/* an NPC was hit */
extern "C" void far HandleNpcHit(objref attacker, objref victim, char hp)
{
	unsigned char alive, survived;
	int side;
	MonsterRef monster;

	monster.index = GetMonsterNumber(victim);
	alive = !IsDying(hp);
	side = ALIGNMENT(NPC(&victim));
	if (RollChance(3) && (char)Item_getHitPoints(&victim) != hp) {
		switch (TYPE(ITEM(victim.off))) {
		case 478: case 354: case 691: case 725: case 744:
			SpriteManager_barkOnItem(&gSpriteManager, victim, GetGameText(1, 204), 1, 15, 0);
			break;
		default:
			if (!(monster->extraFlags & 0x20))
				SpriteManager_barkOnItem(&gSpriteManager, victim,
					GetGameText(1, GenerateRandomIntegerInRange(4) + 41), 1, 15, 0);
		}
	}
	if (!attacker.valid() || !attacker.isNpc()) {
		if (NPC(&victim)->workType == WORK_COMBAT)
			survived = RespondToAttack(victim, -1, hp);
	} else {
		survived = RespondToAttack(victim, NPCNUM(&attacker), hp);
		if (alive && IsDying((char)Item_getHitPoints(&victim)) && !(monster->extraFlags & 0x20))
			SpriteManager_barkOnItem(&gSpriteManager, victim, GetGameText(1, 47), 5, 15, 0);
		if (ALIGNMENT(NPC(&attacker)) != side) {
			if (IsSentient(victim) && IsDying((char)Item_getHitPoints(&victim)) && !IsDead(&victim)
				&& !HasLeader(side) && GenerateRandomIntegerInRange(100) < ProtectChance[side])
				RallyProtectors(victim);
			else if (IsSentient(victim) && GenerateRandomIntegerInRange(100) < CallForHelpChance[side])
				CallForHelp(victim);
		}
	}
	if (IsNpcUnconscious(&victim) && !survived)
		RemoveFromCombat(victim);
}

int far GetNpcWeapon(objref *npc)
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

unsigned far GetRangeWithWeapon(NPCRef &npc, int *weapon)
{
	return GetWeaponRange(npc, *weapon);
}

unsigned far GetAttackRange(NPCRef *npc)
{
	objref item;
	unsigned result;

	item = GetItemInSlot(*npc, 1);
	if (item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */ && (unsigned char)Item_isAvatar(npc))
		result = 9;
	else
		result = GetWeaponRange(*npc, GetNpcWeapon(npc));
	return result;
}

inline unsigned char IsFleeing(objref *npc)
{
	return (unsigned char)Item_getQuality(npc) == ATTACK_FLEE;
}

int far GetCombatTarget(objref *npc)
{
	if (IsFleeing(npc))
		return NPC(npc)->oppressor;
	if ((unsigned char) (NPC(npc)->typeFlagsHigh & 1))
		return NPC(npc)->primaryTarget;
	return NPC(npc)->secondaryTarget;
}

void far CallForHelp(NPCRef &self)
{
	int i;
	int count;
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



void far Npc_setTarget(NPCRef *npc, int target, char primary)
{
	objref victim;
	NPCRef attacker;
	int monster;

	if (primary && (unsigned char)Item_isAvatar(npc))
		RemoveFromCombat(*npc);
	if (!IsInCombat(npc) && target != -1) {
		Npc_setSchedule(npc, WORK_COMBAT);
		CUR_SCHED(npc).state = 1;
	}
	GetNpcIbo(&victim, target);
	if (victim.valid() && (unsigned char) (Item_getQualityFlags(&victim) & QUALITY_INVISIBLE)) {
		monster = MonsterLookup.get(GetItemType(*npc));
		if (!(unsigned char) MonsterRecords.get(monster)->seeInvisible) {
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
unsigned char far PrepareAttack(NPCRef *npc)
{
	objref target;
	WeaponRef weapon;
	int range;
	int damage;
	objref item;
	unsigned char casting = 0;
	unsigned char partyMember;
	char rearm = 0;
	char unconscious;
	char ranged = 0;
	int type;
	int ammoCount;
	struct WeaponRecord details;
	MonsterRecord monster;

	partyMember = IN_PARTY(NPC(npc));
	item = GetItemInSlot(*npc, 1);
	GetNpcIbo(&target, GetCombatTarget(npc));
	if ((unsigned char)Item_isAvatar(npc) && item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */) {
		if (CanCastSpell(*npc, (unsigned char)GetSpellbookBookmark(&NPCRef(item.off)), 1, 1)) {
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
		if (ammoCount == 1 && WeaponRecords.get(weapon)->ammo == -3 && item.valid())
			range = WeaponRecords.get(weapon)->range;
		else
			range = GetAttackRange(npc);
	}
	if (GetMissileDistance(*npc, target) <= range && HasLineOfFire(*npc, target))
		return 1;
	return 0;
}

unsigned char far IsTargetInReach(NPCRef *attacker, objref target)
{
	if (GetMissileDistance(*attacker, target) <= GetAttackRange(attacker)
		&& HasLineOfFire(*attacker, target))
		return 1;
	return 0;
}



unsigned char far FindSpotNextToItem(objref *self, objref item, Coord *outX, Coord *outY, int *outZ)
{
	int baseX, baseY, i, spanX, spanY, z, baseZ;
	int x, y;
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

unsigned char far StepToward(objref *npc, Coord targetX, Coord targetY, int targetZ, int stride)
{
	Coord x, y;
	int dx, dy, sx, sy;
	int i;
	int dz = 0;
	int movement;
	unsigned typeFrame;
	unsigned collision;
	char dir;
	char state = 8;         /* 8 level, 9 climbing, 10 descending */
	char turn;
	unsigned char moved = 0;
	int z;

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
	if ((unsigned char) (NPC(npc)->typeFlags & 0x10) && (char) collision == 0) {
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
	if (!moved && (!(unsigned char) (NPC(npc)->typeFlags & 0x10) || state == 8))
		return 0;
	if ((unsigned char) (NPC(npc)->typeFlags & 0x10)) {
		if (moved)
			StepItem(*npc, dir, (char) collision ? (char) collision : dz, 1);
		else
			StepItem(*npc, 8, dz, 1);
	} else if (!(unsigned char) (NPC(npc)->typeFlags & 0x10))
		StepItem(*npc, dir, (char) collision, stride);
	return 1;
}

unsigned char far ApproachTarget(NPCRef *npc)
{
	NPCRef target;
	Coord x, y;
	int stride;
	unsigned char moved;
	unsigned char pathed;
	int z;

	if (!CanMove(npc))
		return 0;
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
			if ((char) (Npc_getIntelligence(npc) >= 15))
				moved = PathToSpot(npc, x, y, z, 60);
			if (!moved)
				moved = StepToward(npc, x, y, z, stride);
			if (!moved || GetDistance(Item_getX(*npc), Item_getY(*npc), Coord(NPC(npc)->iVr[0]),
				Coord(NPC(npc)->iVr[1])) <= Npc_getMovementRadius(npc)) {
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
			if (!(unsigned char) (NPC(npc)->typeFlags & 0x10) && (char) (Npc_getIntelligence(npc) >= 15)) {
				if (!PathNextToItem(npc, target, 60)) {
					if (!StepToward(npc, x, y, z, stride))
						return 0;
				}
			} else {
				pathed = 0;
				if (!(unsigned char) (NPC(npc)->typeFlags & 0x10) && stride == 1)
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

char far IsRangedAttack(objref *npc, int weaponNumber, objref target)
{
	WeaponRef weapon = weaponNumber;
	int dist;
	int mode;

	if (weapon.none())
		return 0;
	mode = (unsigned char)weapon->uses;
	if (mode == 3)
		return 1;
	if (mode == 0)
		return 0;
	dist = Item_greatestDeltaToItem(*npc, target);
	return weapon->range < dist;
}

int InvisibleTypes[] = { 981, 511, -1 };

char far CanTurnInvisible(objref *p)
{
	int type = TYPE(ITEM(p->off));
	int i;

	for (i = 0; InvisibleTypes[i] != -1; i++)
		if (InvisibleTypes[i] == type) {
			if (Npc_hasNoCastFlag(p))
				return 0;
			return 1;
		}
	return 0;
}

int TeleportingTypes[] = { -1 };

char far CanTeleport(objref *p)
{
	int type = TYPE(ITEM(p->off));
	int i;

	for (i = 0; TeleportingTypes[i] != -1; i++)
		if (TeleportingTypes[i] == type)
			return 1;
	return 0;
}

int SummoningTypes[] = { -1 };

char far CanSummon(objref *p)
{
	int type = TYPE(ITEM(p->off));
	int i;

	for (i = 0; SummoningTypes[i] != -1; i++)
		if (SummoningTypes[i] == type)
			return 1;
	return 0;
}

void far TakeCombatTurn(NPCRef *npc)
{
	unsigned reach;     /* the target's own attack range */
	NPCRef target;
	unsigned range;
	objref weapon = GetNpcWeapon(npc);
	unsigned char atRange;
	int dist;
	int skill;
	int ammoCount = 100;
	int z, dz, newZ, height, targetMiddle;
	char casting = 0;
	char fleeing;
	objref item;

	skill = Npc_getIntelligence(npc);
	GetNpcIbo(&target, GetCombatTarget(npc));
	fleeing = (unsigned char)Item_getQuality(npc) == ATTACK_FLEE;
	if (IN_PARTY(NPC(npc)) && fleeing && ALIGNMENT(NPC(npc)) == ALIGNMENT(NPC(&target))) {
		NPC(npc)->oppressor = -1;
		SpendAttackPoints(npc);
		return;
	}
	item = GetItemInSlot(*npc, 1);
	if (item.valid() && TYPE(ITEM(item.off)) == 761 /* spellbook */ && (unsigned char)Item_isAvatar(npc)) {
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
			&& (dist <= reach && (range > reach || RollChance(2))
			|| RollChance(3) && GenerateRandomIntegerInRange(60) < skill)
				&& TeleportInCombat(npc, fleeing)) {
			CombatGroups.someoneActed = 1;
			SpendAttackPoints(npc);
			return;
		}
		if (range > reach && (dist <= reach || dist + 1 <= range && RollChance(2))
			&& (CombatGroups.movePoints[NPCNUM(npc)] >= CombatGroups.movePoints[NPCNUM(&target)] + 16 || RollChance(2))
			&& (char) (Npc_getIntelligence(npc) >= 18) && FleeStep(npc, fleeing, 1, 0))
			return;
	}
	if (CanAffordAttack(npc) == 0)
		return;
	if (!(unsigned char) (Item_getQualityFlags(npc) & QUALITY_INVISIBLE) && CanTurnInvisible(npc)
		&& GenerateRandomIntegerInRange(375) < skill) {
		SpriteManager_playSpriteForItem(&gSpriteManager, *npc, 0, 0, 0, 0, 1036, 0, -1, 5);
		PlaySoundAtItem(1, *npc);
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
	if ((unsigned char) (NPC(npc)->typeFlags & 0x10) && RollChance(4)) {
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
		if (NPC(&target)->workType == WORK_COMBAT || !IN_PARTY(NPC(&target))) {
			if (NPC(&target)->primaryTarget == NPCNUM(npc))
				NPC(&target)->typeFlagsHigh = NPC(&target)->typeFlagsHigh | 1;
			else if (NPC(&target)->secondaryTarget == NPCNUM(npc))
				NPC(&target)->typeFlagsHigh = NPC(&target)->typeFlagsHigh & ~1;
			else {
				if (NPC(&target)->workType == WORK_SLEEP)
					WakeUpNpc(&target);
				if (ALIGNMENT(NPC(&target)) != ALIGNMENT(NPC(npc))) {
					if (NPC(&target)->workType != WORK_COMBAT && ALIGNMENT(NPC(&target)) == 0
						&& NPCNUM(&target) < 256 && IN_PARTY(NPC(npc)))
{
							unsigned char script[128];
							script[0] = 1;
							AppendScriptByte(script, SCRIPT_NO_HALT);
							AppendScriptByte(script, SCRIPT_USECODE);
							AppendScriptWord(script, 0x63a);
							ActionQueue.add(*npc, (char *)script);
						}
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
		TryToCastSpell(AvatarRef, (unsigned char)GetSpellbookBookmark(&NPCRef(item.off)), 1, 1, 4);
	} else
		AttackItemWithWeapon(*npc, target, weapon);
	if (RollChance(15)) {
		switch (TYPE(ITEM(npc->off))) {
		case 478: case 354: case 691: case 725: case 744:
			SpriteManager_barkOnItem(&gSpriteManager, *npc,
				GetGameText(1, GenerateRandomIntegerInRange(2) + 205), 0, 15, 0);
			break;
		default:
			MonsterRef monster = GetMonsterNumber(*npc);
			if (!(monster->extraFlags & 0x20)) {
				if ((unsigned char)(NPC(&target)->intelligence & 0x1f) < 4)
					SpriteManager_barkOnItem(&gSpriteManager, *npc,
						GetGameText(1, GenerateRandomIntegerInRange(2) + 84), 0, 15, 0);
				else
					SpriteManager_barkOnItem(&gSpriteManager, *npc,
						GetGameText(1, GenerateRandomIntegerInRange(7) + 83), 0, 15, 0);
			}
		}
	}
}

unsigned char far RespondToAttack(NPCRef &self, int attacker, char hp)
{
	objref secondary;
	objref primary;
	NPCRef target;
	objref other;
	char result = 0;
	unsigned char down;
	char calm, ally;
	int align, targetAlign, health;
	unsigned dist;
	int mode;
	int partyIndex;
	MonsterRef monster;
	int type;
	int i;

	monster.index = GetMonsterNumber(self);
	GetNpcIbo(&target, attacker);
	align = ALIGNMENT(NPC(&self));
	health = (char)Item_getHitPoints(&self);
	targetAlign = ALIGNMENT(NPC(&target));
	down = IsNpcUnconscious(&self);
	type = TYPE(ITEM(self.off));
	partyIndex = GetPartyIndex(self);
	if (IN_PARTY(NPC(&self)) && PartyMissileFlags[partyIndex])
		PartyMissileFlags[partyIndex] = 0;
	if (down && (unsigned char)Item_isAvatar(&self) && NPC(&self)->workType)
		BeginCombat();
	if (attacker != -1) {
		if (IN_PARTY(NPC(&target)) && (align == 2 || align == 3) && !CombatGroups.partyAttacked) {
			CombatGroups.partyAttacked = 1;
			CombatGroups.partyHealth = CombatGroups.sumGroupHealth(1);
			CombatGroups.enemyHealth = CombatGroups.sumGroupHealth(2) + CombatGroups.sumGroupHealth(3) + (hp - health);
		}
		if (!IsInCombat(&self)) {
			if (IN_PARTY(NPC(&self)))
				return result;
			if (RollChance(3)) {
				switch (TYPE(ITEM(self.off))) {
				case 478: case 354: case 691: case 725: case 744:
					SpriteManager_barkOnItem(&gSpriteManager, self, GetGameText(1, 207), 2, 15, 0);
					break;
				default:
					if (!(monster->extraFlags & 0x20) && (unsigned char)(NPC(&self)->intelligence & 0x1f) > 3)
						SpriteManager_barkOnItem(&gSpriteManager, self,
							GetGameText(1, GenerateRandomIntegerInRange(2) + 45), 2, 15, 0);
				}
			}
			Npc_setSchedule(&self, WORK_COMBAT);
			CUR_SCHED(&self).state = 1;
			if (IsInMode(&self, ATTACK_FLANK)) {
				SetAttackMode(self, ATTACK_NEAREST);
				result = 1;
			}
			if (align == 0 && NPCNUM(&self) < 256 && IN_PARTY(NPC(&target)))
{
				unsigned char script[128];
				script[0] = 1;
				AppendScriptByte(script, SCRIPT_NO_HALT);
				AppendScriptByte(script, SCRIPT_USECODE);
				AppendScriptWord(script, 0x63a);
				ActionQueue.add(self, (char *)script);
			}
		} else if (IsInMode(&self, ATTACK_FLANK)
			&& (!IsAssigned(NPC(&self)->iVr[0]) || GenerateRandomIntegerInRange(100) < 50)) {
			NPC(&self)->iVr[0] = -1;
			SetAttackMode(self, ATTACK_DEFEND);
			result = 1;
		}
	}
	if (NPC(&self)->workType == WORK_COMBAT) {
		if (!down && attacker != -1 && IsInMode(&self, ATTACK_FLEE) && align != targetAlign) {
			if (!FleeStep(&self, 1, 0, 1)
				&& (!(unsigned char) (Item_getQualityFlags(&target) & QUALITY_INVISIBLE) ||
					(unsigned char) monster->seeInvisible)) {
				dist = GetMissileDistance(self, target);
				if (GetAttackRange(&target) >= dist && GetAttackRange(&self) < dist) {
					if (RollChance(4)) {
						mode = ATTACK_BERSERK;
						switch (TYPE(ITEM(self.off))) {
						case 478: case 354: case 691: case 725: case 744:
							SpriteManager_barkOnItem(&gSpriteManager, self, GetGameText(1, 208), 3, 15, 0);
							break;
						default:
							if (!(monster->extraFlags & 0x20)) {
								if ((unsigned char)(NPC(&self)->intelligence & 0x1f) > 3)
									SpriteManager_barkOnItem(&gSpriteManager, self,
										GetGameText(1, GenerateRandomIntegerInRange(3) + 140), 3, 15, 0);
								else
									SpriteManager_barkOnItem(&gSpriteManager, self, GetGameText(1, 140), 3, 15, 0);
							}
						}
					} else {
						mode = ATTACK_NEAREST;
						switch (TYPE(ITEM(self.off))) {
						case 478: case 354: case 691: case 725: case 744:
							SpriteManager_barkOnItem(&gSpriteManager, self, GetGameText(1, 209), 3, 15, 0);
							break;
						default:
							if (!(monster->extraFlags & 0x20)) {
								if ((unsigned char)(NPC(&self)->intelligence & 0x1f) > 3)
									SpriteManager_barkOnItem(&gSpriteManager, self,
										GetGameText(1, GenerateRandomIntegerInRange(3) + 143), 3, 15, 0);
								else
									SpriteManager_barkOnItem(&gSpriteManager, self, GetGameText(1, 140), 3, 15, 0);
							}
						}
					}
					SetAttackMode(self, mode);
					result = 1;
					Npc_setTarget(&self, attacker, 1);
				}
			}
		} else if (!(unsigned char)Item_isAvatar(&self) && !IsInMode(&self, ATTACK_BERSERK)
			&& !IsInMode(&self, ATTACK_FLEE) &&
			!Npc_hasTournamentFlag(&self) && (unsigned char)(NPC(&self)->strength & 0x1f) / 4 >= health) {
			SetAttackMode(self, ATTACK_FLEE);
			result = 1;
		}
	}
	NPC(&self)->oppressor = attacker;
	if (down)
		return result;
	if (align == targetAlign)
		return result;
	if (!IN_PARTY(NPC(&self)) && !InDungeon && RollChance(3)) {
		if (NPCNUM(&self) < 256) {
			if (ALIGNMENT(NPC(&self)) == 0) {
				switch (TYPE(ITEM(self.off))) {
				case 478: case 354: case 691: case 725: case 744:
					SpriteManager_barkOnItem(&gSpriteManager, self,
						GetGameText(1, GenerateRandomIntegerInRange(2) + 198), 3, 15, 0);
					break;
				default:
					SpriteManager_barkOnItem(&gSpriteManager, self,
						GetGameText(1, GenerateRandomIntegerInRange(5) + 105), 3, 15, 0);
				}
				ally = 0;
				calm = 1;
				for (i = 0; i < NPC_COUNT; i++) {
					GetNpcIbo(&other, i);
					if (other.valid() && CanVisit(&other)) {
						if (NPC(&other)->workType == WORK_COMBAT
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
		} else if ((TYPE(ITEM(self.off)) == 228 || TYPE(ITEM(self.off)) == 259 ||
			TYPE(ITEM(self.off)) == 461 || TYPE(ITEM(self.off)) == 381) && RollChance(3)) {
			if (!(monster->extraFlags & 0x20))
				SpriteManager_barkOnItem(&gSpriteManager, self,
					GetGameText(1, GenerateRandomIntegerInRange(5) + 105), 3, 15, 0);
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
	if (IsSame(NPC(&self)->oppressor, NPC(&self)->primaryTarget) && !(unsigned char) (NPC(&self)->typeFlagsHigh & 1)
		&& (!PrepareAttack(&self) || HasFallen(&secondary) || IsInMode(&secondary, ATTACK_FLEE)))
		NPC(&self)->typeFlagsHigh = NPC(&self)->typeFlagsHigh | 1;
	else if (IsSame(NPC(&self)->oppressor, NPC(&self)->secondaryTarget) &&
		(unsigned char) (NPC(&self)->typeFlagsHigh & 1) && !IsInMode(&self, ATTACK_PROTECT)
		&& (!IsAssigned(NPC(&self)->primaryTarget) || !PrepareAttack(&self) || HasFallen(&primary)
			|| IsInMode(&primary, ATTACK_FLEE)))
		NPC(&self)->typeFlagsHigh = NPC(&self)->typeFlagsHigh & ~1;
	return result;
}



unsigned char far FleeStep(objref *npc, char panic, char spend, char mayTeleport)
{
	AreaSearch inventory, contents;
	objref target;
	int i;
	int first;
	unsigned char dir;
	unsigned char step;
	unsigned collision;
	int stride;
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
				Coord(Item_getY(*npc) + DirDeltaY[step]), Item_getZ(npc) + (int) (char) collision))) {
			AddTypeToCollision(*npc);
			if (spend) {
				CombatGroups.someoneActed = 1;
				CombatGroups.movePoints[NPCNUM(npc)] -= panic == 1 ? 12 : 16;
				StepItem(*npc, step, (char) collision, stride);
				/* a bleeding NPC in a panic leaves blood and drops things */
				if (panic && (unsigned char) (NPC(npc)->typeFlagsHigh & 4)) {
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
					if (RollChance(25) && !Npc_hasTournamentFlag(npc)) {
						item.off = 0;
						if (IN_PARTY(NPC(npc)) == 0) {
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
							if ((unsigned char) ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(item.off))))->spell)
								Item_delete(&item);
							else {
								Item_move(&item, Coord(Item_getX(*npc) + DirDeltaX[dir]),
									Coord(Item_getY(*npc) + DirDeltaY[dir]), Item_getZ(npc));
								Item_setOkayToTake(&item);
								FindItemInContainer(&contents, item, 0, -1, 0xff, 0xff);
								while (contents.current.valid()) {
									Item_setOkayToTake(&contents.current);
									if ((unsigned char) ReadyRecords.get(
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

unsigned char far CanMove(objref *p)
{
	int flags = NPC(p)->typeFlags;

	if (flags & 0x20 || flags & 0x10 || flags & 0x40 || flags & 0x80)
		return 1;
	return 0;
}

inline unsigned char IsMultipart(objref *p)
{
	return gItemTypeInfo[TYPE(ITEM(p->off))].strangeMovement;
}

int far GetMoveStride(objref *p)
{
	if (IsMultipart(p) && HasEvenCellPlacement(TYPE(ITEM(p->off))))
		return 2;
	return 1;
}



int far Npc_getScheduleVariable0(objref *npc)
{
	return NPC(npc)->sVr[0];
}

int far Npc_getScheduleVariable1(objref *npc)
{
	return NPC(npc)->sVr[1];
}

unsigned char far Npc_getMovementRadius(objref *npc)
{
	return NPC(npc)->typeFlags & 7;
}
