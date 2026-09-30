/* Black Gate U7.EXE, overlay segment 216 (file offsets 0x055470 to 0x056d3f, 6351 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

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
	int index;
	WeaponRef() {}
	WeaponRef(int n) { index = n; }
	void operator=(int n) { index = n; }
	unsigned char isNull() { return index == 0; }
	unsigned char isNone() { return index == -1; }
};
struct AmmoRef {
	int index;
	AmmoRef() {}
	void operator=(int n) { index = n; }
	unsigned char isNull() { return index == 0; }
};

struct Script {
	unsigned char length, bytes[127];
	Script() { length = 1; }
	void append(int code) { AppendScriptByte(&length, code); }
};

int StrikeTargetX, StrikeTargetY, StrikeTargetZ;
int AttackTargetZ;
extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern "C" int far Item_greatestDeltaToItem(objref &ref, objref other);
extern int far Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, unsigned targetZ);
extern unsigned char far Item_getQualityFlags(objref *);
extern unsigned char far Item_getDirToItem(objref *, objref, unsigned char);
extern unsigned char far Item_getDirToCoords(objref *, Coord, Coord, unsigned char);
extern int far FindWeaponAmmo(objref, int, objref far &);
extern unsigned char far Item_getCharges(objref *ref);
extern void far Item_setQuality(objref *, char);
extern unsigned char far Item_getQuality(objref *ref);
extern unsigned char far Item_getQuantity(objref *ref);
extern char far Item_delete(objref *ref);
extern unsigned char far Item_moveIntoContainer(objref *ref, objref container);
extern unsigned char far CreateItem(objref *, TypeFrame);
extern unsigned char far CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int);
extern void far Item_storeQuantity(objref *, unsigned char);
extern void far Item_setTemporary(objref *);
extern "C" unsigned char far RollToHit(objref, objref, int);
extern "C" void far PlaySoundAtItem(int, ItemId);

objref AttackTargetItem;
Coord AttackTargetX, AttackTargetY;
objref StrikeTargetItem;

static char far PostAttackScript(objref actor, int number);
static char far ResolveAttack(objref actor, int number);

char far AttackItem(objref actor, objref target)
{
	WeaponRef weapon;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	AttackTargetItem = target;
	return PostAttackScript(actor, weapon.index);
}

char far AttackCoords(objref actor, Coord x, Coord y, int z)
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

char far AttackItemWithWeapon(objref actor, objref target, int weapon)
{
	AttackTargetItem = target;
	return PostAttackScript(actor, weapon);
}

char far AttackCoordsWithWeapon(objref actor, Coord x, Coord y, int z, int weapon)
{
	AttackTargetItem = 0;
	AttackTargetX = x;
	AttackTargetY = y;
	AttackTargetZ = z;
	return PostAttackScript(actor, weapon);
}

static char far PostAttackScript(objref actor, int number)
{
	WeaponRef weapon = number;
	unsigned char ranged;
	int delta;
	int range;
	int monsterType;
	char sound;
	WeaponRecord details;
	Script script;

	if (AttackTargetItem.valid())
		delta = (unsigned)Item_greatestDeltaToItem(actor, AttackTargetItem);
	else
		delta = Item_greatestDeltaToCoords(actor, AttackTargetX, AttackTargetY, AttackTargetZ);
	if ((unsigned char)(Item_getQualityFlags(&actor) & QUALITY_BUSY))
		return 0;
	if (weapon.isNone()) {
		int monster = MonsterLookup.get(actor.type());
		range = (unsigned char)MonsterRecords.get(monster)->range;
		sound = MonsterRecords.get(monster)->attackSound();
		int index = 0;
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
		script.append((unsigned char)(AttackTargetItem.valid() ? Item_getDirToItem(&actor, AttackTargetItem, 1)
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
	if ((unsigned char)ReadyRecords.get(ReadyLookup.get(details.type))->slot != 20) {
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

char far StrikeItem(objref actor, objref target)
{
	WeaponRef weapon;
	if (!target.valid())
		return 0;
	weapon = GetWeaponNumber(actor);
	if (weapon.isNull())
		weapon = -1;
	StrikeTargetItem = target;
	StrikeTargetX = Coord(Item_getX(target) + (GetFootprintX(*(TypeFrame far *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetY = Coord(Item_getY(target) + (GetFootprintY(*(TypeFrame far *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetZ = Item_getZ(&target) + gItemTypeInfo[target.type()].height / 2;
	return ResolveAttack(actor, weapon.index);
}

char far StrikeCoords(objref actor, Coord x, Coord y, int z)
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

unsigned char far StrikeItemWithWeapon(objref actor, objref target, int weapon)
{
	if (!target.valid())
		return 0;
	StrikeTargetItem = target;
	StrikeTargetX = Coord(Item_getX(target) + (GetFootprintX(*(TypeFrame far *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetY = Coord(Item_getY(target) + (GetFootprintY(*(TypeFrame far *)&target.ptr()->typeFrame) >> 1));
	StrikeTargetZ = Item_getZ(&target) + gItemTypeInfo[target.type()].height / 2;
	return ResolveAttack(actor, weapon);
}

char far StrikeCoordsWithWeapon(objref actor, Coord x, Coord y, int z, int weapon)
{
	StrikeTargetItem = 0;
	StrikeTargetX = x;
	StrikeTargetY = y;
	StrikeTargetZ = z;
	return ResolveAttack(actor, weapon);
}

static char far ResolveAttack(objref actor, int number)
{
	WeaponRef weapon = number;
	unsigned char ranged;
	unsigned char useAmmo;
	objref ammoItem;
	int delta;
	int range;
	int damage;
	AmmoRef ammoIndex;
	objref missile;
	int projectile;
	unsigned char kind;
	unsigned char checkLineOfFire;
	unsigned char returns;
	int bonus;
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
		delta = (unsigned)Item_greatestDeltaToItem(actor, StrikeTargetItem);
	} else
		delta = Item_greatestDeltaToCoords(actor, StrikeTargetX, StrikeTargetY, StrikeTargetZ);
	if (weapon.isNone()) {
		int monster = MonsterLookup.get(actor.type());
		range = (unsigned char)MonsterRecords.get(monster)->range;
		damage = (unsigned char)MonsterRecords.get(monster)->damage;
		projectile = -1;
		int index = 0;
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
	if (useAmmo && (unsigned)details.ammo < 32768) {
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
				unsigned char worn = GetItemZAndStuff(&ammoItem).kind() == LOCATION_EQUIPPED;
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
