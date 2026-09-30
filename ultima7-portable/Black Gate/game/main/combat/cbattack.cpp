/* Black Gate U7.EXE, overlay segment 216 (file offsets 0x055470 to 0x056d3f, 6351 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "typefram.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "coord.h"
#include "makemojo.h"
#include "monsters.h"
#include "script.h"
#include "actqueue.h"
#include "cast.h"
#include "combwpn.h"
#include "missile.h"
#include "misstrac.h"
#include "text.h"
#include "explode.h"
#include "combat.h"
#include "u7sound.h"
#include "wihh.h"
#include "type.h"
#include "objref.h"
#include "npcref.h"
#include "voolook.h"
#include "ready.h"
#include "weapons.h"
#include "ammo.h"
#include "item.h"

struct WeaponRef {
	int16_t index;
	WeaponRef() {}
	WeaponRef(int16_t n) { index = n; }
	void operator=(int16_t n) { index = n; }
	uint8_t isNull() { return index == 0; }
	uint8_t isNone() { return index == -1; }
};
struct AmmoRef {
	int16_t index;
	AmmoRef() {}
	void operator=(int16_t n) { index = n; }
	uint8_t isNull() { return index == 0; }
};

struct Script {
	uint8_t length, bytes[127];
	Script() { length = 1; }
	void append(int16_t code) { AppendScriptByte(&length, code); }
};

int16_t StrikeTargetX, StrikeTargetY, StrikeTargetZ;
int16_t AttackTargetZ;
extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern "C" int16_t Item_greatestDeltaToItem(objref &ref, objref other);
extern int16_t Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, uint16_t targetZ);
extern uint8_t Item_getQualityFlags(objref *);
extern uint8_t Item_getDirToItem(objref *, objref, uint8_t);
extern uint8_t Item_getDirToCoords(objref *, Coord, Coord, uint8_t);
extern int16_t FindWeaponAmmo(objref, int16_t, objref &);
extern uint8_t Item_getCharges(objref *ref);
extern void Item_setQuality(objref *, int8_t);
extern uint8_t Item_getQuality(objref *ref);
extern uint8_t Item_getQuantity(objref *ref);
extern int8_t Item_delete(objref *ref);
extern uint8_t Item_moveIntoContainer(objref *ref, objref container);
extern uint8_t CreateItem(objref *, TypeFrame);
extern uint8_t CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int16_t);
extern void Item_storeQuantity(objref *, uint8_t);
extern void Item_setTemporary(objref *);
extern "C" uint8_t RollToHit(objref, objref, int16_t);
extern "C" void PlaySoundAtItem(int16_t, ItemId);

objref AttackTargetItem;
Coord AttackTargetX, AttackTargetY;
objref StrikeTargetItem;

static int8_t PostAttackScript(objref actor, int16_t number);
static int8_t ResolveAttack(objref actor, int16_t number);

int8_t AttackItem(objref actor, objref target)
{
	WeaponRef weapon;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	AttackTargetItem = target;
	return PostAttackScript(actor, weapon.index);
}

int8_t AttackCoords(objref actor, Coord x, Coord y, int16_t z)
{
	WeaponRef weapon;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	AttackTargetItem = 0;
	AttackTargetX = x;
	AttackTargetY = y;
	AttackTargetZ = z;
	return PostAttackScript(actor, weapon.index);
}

int8_t AttackItemWithWeapon(objref actor, objref target, int16_t weapon)
{
	AttackTargetItem = target;
	return PostAttackScript(actor, weapon);
}

int8_t AttackCoordsWithWeapon(objref actor, Coord x, Coord y, int16_t z, int16_t weapon)
{
	AttackTargetItem = 0;
	AttackTargetX = x;
	AttackTargetY = y;
	AttackTargetZ = z;
	return PostAttackScript(actor, weapon);
}

static int8_t PostAttackScript(objref actor, int16_t number)
{
	WeaponRef weapon = number;
	uint8_t ranged;
	int16_t delta;
	int16_t range;
	int16_t monsterType;
	int8_t sound;
	WeaponRecord details;
	Script script;

	if (AttackTargetItem.valid())
		delta = (uint16_t)Item_greatestDeltaToItem(actor, AttackTargetItem);
	else
		delta = Item_greatestDeltaToCoords(actor, AttackTargetX, AttackTargetY, AttackTargetZ);
	if ((uint8_t)(Item_getQualityFlags(&actor) & QUALITY_BUSY))
		return 0;
	if (weapon.isNone()) {
		int16_t monster = MonsterLookup.get(actor.type());
		range = (uint8_t)MonsterRecords.get(monster)->range;
		sound = MonsterRecords.get(monster)->attackSound();
		int16_t index = 0;
		WeaponRecords.read(index, &details);
	} else {
		WeaponRecords.read(weapon.index, &details);
		range = details.range;
		sound = details.sound;
		if (details.homing && !AttackTargetItem.valid()) {
			if (actor == AvatarRef)
				ReportNoCanDo(1);
			return 0;
		}
	}
	ranged = details.uses == 3 || (details.uses != 0 && delta > range);
	monsterType = MonsterLookup.get(actor.type());
	if (GetNpcBufferForIbo(&actor)->typeFlags & 0xf0) {
		script.append(SCRIPT_FACE);
		script.append((uint8_t)(AttackTargetItem.valid() ? Item_getDirToItem(&actor, AttackTargetItem, 1)
			: Item_getDirToCoords(&actor, AttackTargetX, AttackTargetY, 1)) + '0');
	}
	script.append(SCRIPT_READY_FRAME);
	if (ranged && !details.rangedReadyFrame)
		script.append(SCRIPT_READY_FRAME);
	if (ranged && !details.rangedStrikeFrame)
		script.append(SCRIPT_READY_FRAME);
	if (!ranged && !details.meleeReadyFrame)
		script.append(SCRIPT_READY_FRAME);
	if (!ranged && !details.meleeStrikeFrame)
		script.append(SCRIPT_READY_FRAME);
	/* slot 20 weapons swing with the two-handed frames */
	if ((uint8_t)ReadyRecords.get(ReadyLookup.get(details.type))->slot != 20) {
		if (ranged) {
			if (details.rangedReadyFrame)
				script.append(SCRIPT_RAISE1_FRAME);
			if (details.rangedStrikeFrame)
				script.append(SCRIPT_EXTEND1_FRAME);
		} else {
			if (details.meleeReadyFrame)
				script.append(SCRIPT_RAISE1_FRAME);
			if (sound) {
				script.append(SCRIPT_SFX);
				script.append(sound - 1);
				script.append(1);
			}
			if (details.meleeStrikeFrame)
				script.append(SCRIPT_EXTEND1_FRAME);
		}
		script.append(SCRIPT_THRUST1_FRAME);
	} else {
		if (ranged) {
			if (details.rangedReadyFrame)
				script.append(SCRIPT_RAISE2_FRAME);
			if (details.rangedStrikeFrame)
				script.append(SCRIPT_EXTEND2_FRAME);
		} else {
			if (details.meleeReadyFrame)
				script.append(SCRIPT_RAISE2_FRAME);
			if (sound) {
				script.append(SCRIPT_SFX);
				script.append(sound - 1);
				script.append(1);
			}
			if (details.meleeStrikeFrame)
				script.append(SCRIPT_EXTEND2_FRAME);
		}
		script.append(SCRIPT_THRUST2_FRAME);
	}
	if (ranged && sound) {
		script.append(SCRIPT_SFX);
		script.append(sound - 1);
		script.append(1);
	}
	script.append(122);        /* strike */
	script.append(SCRIPT_READY_FRAME);
	if (AttackTargetItem.valid())
		Npc_setItemTarget(&actor, AttackTargetItem.off);
	else
		Npc_setCoordTarget(&actor, AttackTargetX, AttackTargetY, AttackTargetZ);
	Npc_setTargetWeapon(&actor, weapon.index);
	ActionQueue.add(actor.off, (char *)&script);
	return 1;
}

int8_t StrikeItem(objref actor, objref target)
{
	WeaponRef weapon;
	if (!target.valid())
		return 0;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	StrikeTargetItem = target;
	StrikeTargetX = Coord(Item_getX(target) + (GetFootprintX(*(TypeFrame *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetY = Coord(Item_getY(target) + (GetFootprintY(*(TypeFrame *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetZ = Item_getZ(&target) + gItemTypeInfo[target.type()].height / 2;
	return ResolveAttack(actor, weapon.index);
}

int8_t StrikeCoords(objref actor, Coord x, Coord y, int16_t z)
{
	WeaponRef weapon;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	StrikeTargetItem = 0;
	StrikeTargetX = x;
	StrikeTargetY = y;
	StrikeTargetZ = z;
	return ResolveAttack(actor, weapon.index);
}

uint8_t StrikeItemWithWeapon(objref actor, objref target, int16_t weapon)
{
	if (!target.valid())
		return 0;
	StrikeTargetItem = target;
	StrikeTargetX = Coord(Item_getX(target) + (GetFootprintX(*(TypeFrame *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetY = Coord(Item_getY(target) + (GetFootprintY(*(TypeFrame *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetZ = Item_getZ(&target) + gItemTypeInfo[target.type()].height / 2;
	return ResolveAttack(actor, weapon);
}

int8_t StrikeCoordsWithWeapon(objref actor, Coord x, Coord y, int16_t z, int16_t weapon)
{
	StrikeTargetItem = 0;
	StrikeTargetX = x;
	StrikeTargetY = y;
	StrikeTargetZ = z;
	return ResolveAttack(actor, weapon);
}

static int8_t ResolveAttack(objref actor, int16_t number)
{
	WeaponRef weapon = number;
	uint8_t ranged;
	uint8_t useAmmo;
	objref ammoItem;
	int16_t delta;
	int16_t range;
	int16_t damage;
	AmmoRef ammoIndex;
	objref missile;
	int16_t projectile;
	uint8_t kind;
	uint8_t checkLineOfFire;
	uint8_t returns;
	int16_t bonus;
	AmmoRecord ammo;
	WeaponRecord details;

	if (!SpecialMusicPlaying || CurrentMusic == 10)
		PlayCombatMusic();
	bonus = 0;
	checkLineOfFire = 0;
	returns = 0;
	if (StrikeTargetItem.valid()) {
		if (StrikeTargetItem.isNpc() && NPCRef(StrikeTargetItem).flag(NPC_DEAD))
			return 0;
		delta = (uint16_t)Item_greatestDeltaToItem(actor, StrikeTargetItem);
	} else
		delta = Item_greatestDeltaToCoords(actor, StrikeTargetX, StrikeTargetY, StrikeTargetZ);
	if (weapon.isNone()) {
		int16_t monster = MonsterLookup.get(actor.type());
		range = (uint8_t)MonsterRecords.get(monster)->range;
		damage = (uint8_t)MonsterRecords.get(monster)->damage;
		projectile = -1;
		int16_t index = 0;
		WeaponRecords.read(index, &details);
	} else {
		WeaponRecords.read(weapon.index, &details);
		range = details.range;
		damage = details.damage;
		projectile = details.projectile;
		if (projectile == -3)
			projectile = details.type;
		if (details.passesBlockers)
			checkLineOfFire = 0;
		if (details.returns)
			returns = 1;
	}
	ranged = details.uses == 3 || delta > range;
	useAmmo = details.ammo != -1 && ranged || details.uses == 0 && details.ammo == -2;
	if ((details.uses == 0 || details.uses == 3) && delta > range) {
		if (actor == AvatarRef && !IsAvatarAutoAttack())
			ReportNoCanDo(2);
		return 0;
	}
	if (useAmmo && !FindWeaponAmmo(actor, weapon.index, ammoItem)) {
		if (actor == AvatarRef)
			ReportNoCanDo(3);
		return 0;
	}
	/* a real ammunition type rather than one of the negative codes */
	if (useAmmo && (uint16_t)details.ammo < 32768) {
		ammoIndex = AmmoLookup.get(ammoItem.type());
		AmmoRecords.read(ammoIndex.index, &ammo);
		if (!ammoIndex.isNull()) {
			damage += AmmoRecords.get(ammoIndex.index)->damage;
			if (ammo.projectile == -3)
				projectile = ammo.type;
			else if (ammo.projectile != -1 && ammo.projectile != ammo.family)
				projectile = ammo.projectile;
			if (ammo.returns)
				returns = 1;
		}
	} else {
		ammoIndex = AmmoLookup.get(details.type);
		AmmoRecords.read(ammoIndex.index, &ammo);
		if (!ammoIndex.isNull()) {
			damage += AmmoRecords.get(ammoIndex.index)->damage;
			if (ammo.projectile == -3)
				projectile = ammo.type;
			else if (ammo.projectile != -1 && ammo.projectile != ammo.family)
				projectile = ammo.projectile;
		}
	}
	if (projectile == -1)
		projectile = 0;
	if (ranged) {
		if (CreateItem(&missile, projectile, Item_getX(actor), Item_getY(actor), 0)) {
			Item_storeQuantity(&missile, 1);
			Item_setTemporary(&missile);
		}
	}
	if (useAmmo) {
		if (details.ammo == -2) {
			Item_setQuality(&ammoItem, Item_getCharges(&ammoItem) - 1);
			if (details.consumed && Item_getQuality(&ammoItem) == 0) {
				Item_delete(&ammoItem);
				if (actor.valid())
					SelectWeapon(actor, 0, 0);
			}
			if (ranged)
				Item_setQuality(&missile, 0);
		} else if (details.projectile == 948) {        /* a triple crossbow fires three bolts */
			if (Item_getQuantity(&ammoItem) > 3) {
				Item_setQuantity(ammoItem, Item_getQuantity(&ammoItem) - 3, 0);
			} else if (objref(GetItemInSlot(actor, 8)) == ammoItem) {
				Item_moveIntoContainer(&ammoItem, actor);
				goto destroy;
			} else
				Item_delete(&ammoItem);
		} else {
			if (Item_getQuantity(&ammoItem) > 1) {
				Item_setQuantity(ammoItem, Item_getQuantity(&ammoItem) - 1, 0);
			} else if (details.ammo == -3) {
				uint8_t worn = GetItemZAndStuff(&ammoItem).kind() == LOCATION_EQUIPPED;
				if (worn)
					Item_moveIntoContainer(&ammoItem, actor);
				Item_delete(&ammoItem);
				if (worn && !returns)
					SelectWeapon(actor, 0, 0);
			} else {
				if (objref(GetItemInSlot(actor, 8)) == ammoItem)
					Item_moveIntoContainer(&ammoItem, actor);
			destroy:
				Item_delete(&ammoItem);
			}
		}
	}
	if (details.lucky)
		bonus += 3;
	if (ammo.lucky)
		bonus += 3;
	if (ranged) {
		if (details.missileSpeed == 0) {
			if (details.speed == 0)
				kind = 2;
			else if (details.speed > 2)
				kind = 0;
			else
				kind = 1;
			if (StrikeTargetItem.valid())
				return FireMissileAtItem(missile, weapon.index, ammoIndex.index, actor,
					Npc_getCombat(&actor) + bonus + 6, StrikeTargetItem, kind);
			return FireMissileAtCoords(missile, weapon.index, ammoIndex.index, actor,
				Npc_getCombat(&actor) + bonus + 6, Coord(StrikeTargetX), Coord(StrikeTargetY), StrikeTargetZ, kind);
		} else {
			switch (details.uses) {
			case 2:
				bonus -= delta / 2;
				break;
			case 1:
				bonus -= delta;
				break;
			}
			if (StrikeTargetItem.valid())
				return FireMissileAtItem(missile, weapon.index, ammoIndex.index, actor,
					Npc_getCombat(&actor) + bonus + 6, StrikeTargetItem, 3);
			return FireMissileAtCoords(missile, weapon.index, ammoIndex.index, actor,
				Npc_getCombat(&actor) + bonus + 6, Coord(StrikeTargetX), Coord(StrikeTargetY), StrikeTargetZ, 3);
		}
	}
	if (StrikeTargetItem.valid() && checkLineOfFire) {
		if (!HasLineOfFire(actor, StrikeTargetItem)) {
			if (actor == AvatarRef)
				ReportNoCanDo(0);
			return 0;
		}
	}
	if (!StrikeTargetItem.valid() ||
		!details.autoHit && !ammo.autoHit && !RollToHit(actor, StrikeTargetItem, bonus))
		return 0;
	if (details.damage)
		PlaySoundAtItem(details.type == 604 ? 37 : 4, StrikeTargetItem);    /* Glass Sword */
	if (weapon.isNone())
		ApplyMonsterHit(actor, StrikeTargetItem, damage);
	else if (details.type == 704)   /* powder keg */
		ExplodePowderKeg(StrikeTargetItem);
	else
		ApplyWeaponHit(actor, StrikeTargetItem, number, ammoIndex.index, 0, 0);
	return 1;
}

extern "C" void ResetCbattackGlobals(void)
{
	StrikeTargetX = 0;
	StrikeTargetY = 0;
	StrikeTargetZ = 0;
	AttackTargetZ = 0;
	memset((void *)&AttackTargetItem, 0, sizeof(AttackTargetItem));
	memset((void *)&AttackTargetX, 0, sizeof(AttackTargetX));
	memset((void *)&AttackTargetY, 0, sizeof(AttackTargetY));
	memset((void *)&StrikeTargetItem, 0, sizeof(StrikeTargetItem));
}
