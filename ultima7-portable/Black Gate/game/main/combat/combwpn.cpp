/* Black Gate U7.EXE, overlay segment 222 (file offsets 0x062290 to 0x0639e4, 5972 bytes).
 * Borland C++ 2.0 -mm -O -P -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "typefram.h"
#include "lowlevel.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "objref.h"
#include "u7npc.h"
#include "coord.h"
#include "daze.h"
#include "makemojo.h"
#include "random.h"
#include "actqueue.h"
#include "death.h"
#include "explode.h"
#include "slime.h"
#include "special.h"
#include "u7sound.h"
#include "usehook.h"
#include "script.h"
#include "type.h"
#include "npcref.h"
#include "damage.h"
#include "item.h"
#include "weapons.h"
#include "ammo.h"

struct WeaponRef {
	int16_t index;
	WeaponRef(int16_t n) { index = n; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	uint8_t none() { return index == -1; }
};
struct AmmoRef {
	int16_t index;
	AmmoRef(int16_t n) { index = n; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
	uint8_t none() { return index == 0; }
	uint8_t damageType() { return AmmoRecords.get(index)->damageType; }
	uint8_t explodes() { return AmmoRecords.get(index)->explodes; }
	uint8_t sleep() { return AmmoRecords.get(index)->sleep; }
	uint8_t charm() { return AmmoRecords.get(index)->charm; }
	uint8_t curse() { return AmmoRecords.get(index)->curse; }
	uint8_t poison() { return AmmoRecords.get(index)->poison; }
	uint8_t paralyze() { return AmmoRecords.get(index)->paralyze; }
	uint8_t drainMana() { return AmmoRecords.get(index)->drainMana; }
	uint8_t drainHealth() { return AmmoRecords.get(index)->drainHealth; }
	uint8_t noDamage() { return AmmoRecords.get(index)->noDamage; }
};

objref HitTargetItem;
Coord HitX, HitY;
int16_t HitMissile;
uint8_t HitFromExplosion;
int16_t NaturalDamage, HitZ, HitSourceItem;

static void ApplyHit(objref attacker, int16_t weaponNumber, int16_t ammoNumber);

void ApplyMonsterHit(objref attacker, objref target, int8_t damage)
{
	NaturalDamage = damage;
	HitTargetItem = target;
	HitX = Coord(Item_getX(target) + (GetFootprintX(target.ptr()->asTypeFrame()) >> 1));
	HitY = Coord(Item_getY(target) + (GetFootprintY(target.ptr()->asTypeFrame()) >> 1));
	HitZ = Item_getZ(&target) + (gItemTypeInfo[target.type()].height >> 1);
	HitFromExplosion = 0;
	if (attacker.valid() && !attacker.isNpc())
		HitSourceItem = attacker;
	else
		HitSourceItem = 0;
	ApplyHit(attacker, -1, 0);
}

void ApplyHitAtCoords(objref attacker, Coord x, Coord y, int16_t z, int16_t weapon, int16_t ammo, int16_t missile)
{
	HitTargetItem = 0;
	HitX = x;
	HitY = y;
	HitZ = z;
	HitFromExplosion = 0;
	if (missile == 0 && attacker.valid() && !attacker.isNpc()) {
		HitSourceItem = attacker;
		attacker = AvatarRef;
	} else {
		HitSourceItem = missile;
	}
	ApplyHit(attacker, weapon, ammo);
}

void ApplyWeaponHit(objref attacker, objref target, int16_t weapon, int16_t ammo, int16_t missile, uint8_t fromExplosion)
{
	HitTargetItem = target;
	HitX = Coord(Item_getX(target) - (GetFootprintX(target.ptr()->asTypeFrame()) >> 1));
	HitY = Coord(Item_getY(target) - (GetFootprintY(target.ptr()->asTypeFrame()) >> 1));
	HitZ = Item_getZ(&target) + (gItemTypeInfo[target.type()].height >> 1);
	HitSourceItem = missile;
	HitFromExplosion = fromExplosion;
	if (missile == 0 && attacker.valid() && !attacker.isNpc()) {
		HitSourceItem = attacker;
		attacker = AvatarRef;
	} else {
		HitSourceItem = missile;
	}
	ApplyHit(attacker, weapon, ammo);
}

static void ApplyHit(objref attacker, int16_t weaponNumber, int16_t ammoNumber)
{
	WeaponRef weapon = weaponNumber;
	AmmoRef ammo = ammoNumber;
	NPCRef self;
	uint8_t damageType, explodes;
	int16_t event;
	uint8_t strength, intelligence;
	int16_t result, resist;
	uint8_t sleep, charm, curse, poison, paralyze, drainMana, drainHealth, effect7;
	uint8_t alignment, targetAlignment;
	int16_t damage;

	if (weaponNumber == 0 && ammoNumber == 0)
		return;
	resist = 16;
	if (!attacker.isNpc() && !weapon.none()) {
		if (weapon->type == 702 /* cannon */)
			attacker = AvatarRef;
	}
	if (attacker.isNpc()) {
		self.off = attacker.off;
		strength = self.strength();
		intelligence = self.intelligence();
		alignment = (self.buffer()->status & 0x18) >> 3;
	} else {
		(objref &)self = objref(0);
		strength = 0;
		intelligence = 16;
		alignment = 3;
	}
	if (weapon.none()) {
		damage = NaturalDamage;
		damageType = 0;
		explodes = 0;
		sleep = charm = curse = poison = paralyze = drainMana = drainHealth = effect7 = 0;
		event = 0;
	} else {
		damage = weapon->damage;
		damageType = weapon->damageType;
		explodes = weapon->explodes;
		event = weapon->usecode;
		sleep = weapon->sleep;
		charm = weapon->charm;
		curse = weapon->curse;
		poison = weapon->poison;
		paralyze = weapon->paralyze;
		drainMana = weapon->drainMana;
		drainHealth = weapon->drainHealth;
		effect7 = weapon->noDamage;
		if (!ammo.none()) {
			damage += ammo->damage;
			if (ammo.damageType())
				damageType = ammo.damageType();
			if (ammo.explodes())
				explodes = 1;
			if (ammo.sleep())
				sleep = 1;
			if (ammo.charm())
				charm = 1;
			if (ammo.curse())
				curse = 1;
			if (ammo.poison())
				poison = 1;
			if (ammo.paralyze())
				paralyze = 1;
			if (ammo.drainMana())
				drainMana = 1;
			if (ammo.drainHealth())
				drainHealth = 1;
			if (ammo.noDamage())
				effect7 = 1;
		}
	}
	if (!HitFromExplosion && explodes) {
		extern int16_t HitMissile;     /* redeclared on purpose: it changes the generated code */

		if (HitSourceItem != 0) {
			HitMissile = HitSourceItem;
			Item_detach(&objref(HitMissile));
		}
		Explode(attacker, HitX, HitY, HitZ, weaponNumber, ammoNumber, HitSourceItem);
		if (HitMissile != 0) {
			PlaceItem(&objref(HitMissile), HitX, HitY, HitZ);
			HitMissile = 0;
		}
	} else {
		if (HitTargetItem.valid()) {
			/* the initial alignment, so a charmed NPC counts for its own side */
			if (HitTargetItem.isNpc())
				targetAlignment = (NPCRef(HitTargetItem).buffer()->status & 0x60) >> 5;
			else
				targetAlignment = 4;
			if (weapon.none()) {
				result = DealDamage(strength, damage, 0, HitTargetItem, self);
			} else if (damage == 0) {
				result = -1;
				resist = intelligence;
			} else
				result = DealDamage(strength, damage, damageType, HitTargetItem, self);
			if (HitTargetItem.isNpc() && result != 0) {
				if (HitTargetItem.multipart()) {
					HitTargetItem = GetStrangeMoverTarget(HitTargetItem);
				}
				NPCRef npc = HitTargetItem;
				/* protection wards off the weapon's effects */
				if (!npc.flag(NPC_PROTECTED)) {
					if (sleep && !RollToWin(npc.intelligence(), resist))
						PutToSleep(HitTargetItem);
					if (charm && !RollToWin(npc.intelligence(), resist))
						ApplyCharm(&NPCRef(HitTargetItem), alignment);
					if (curse && !RollToWin(npc.intelligence(), resist))
						ApplyCurse(HitTargetItem);
					if (poison && !RollToWin(npc.strength(), resist))
						ApplyPoison(HitTargetItem);
					if (paralyze && !RollToWin(npc.strength(), resist))
						ApplyParalysis(HitTargetItem);
					if (drainMana && (uint8_t)(gItemTypeInfo[HitTargetItem.type()].typeClass == TYPE_CLASS_HUMAN))
						npc.buffer()->mana = RandomBetween(0, npc.buffer()->mana);
					if (drainHealth
						&& (uint8_t)(gItemTypeInfo[HitTargetItem.type()].typeClass == TYPE_CLASS_HUMAN))
						npc.buffer()->hitPoints = RandomBetween(0, npc.buffer()->hitPoints);
				}
				if (damage > 0 && !(Item_getQualityFlags(&HitTargetItem) & QUALITY_BUSY)) {
					if ((HitTargetItem.multipart() && IsInSleepFrame(HitTargetItem)) ||
						(!HitTargetItem.multipart() && (self.frame() == 13 || self.frame() == 29)))
						ActionQueue.add(HitTargetItem, (char *)MakeScript(SCRIPT_BEND_FRAME, SCRIPT_STAND_FRAME,
							SCRIPT_END));
				}
			}
			if (result != 0 && result != -1) {
				if (targetAlignment != 4)
					NoteCombatAlignment(targetAlignment);
				if (attacker.isNpc())
					AwardExperience(self, (int32_t)result);
			}
		}
		if (!weapon.none() && event != 0)
			RunUsable(4, HitTargetItem, event);
	}
}

void BreakHitItem(objref ref)
{
	int16_t type = ref.type();
	if (type == 704 /* powder keg */)
		ExplodePowderKeg(ref);
	else if (!PowderKegExploding)
		RunUsable(4, ref, 0x626);
}

extern "C" void ResetCombwpnGlobals(void)
{
	memset((void *)&HitTargetItem, 0, sizeof(HitTargetItem));
	memset((void *)&HitX, 0, sizeof(HitX));
	memset((void *)&HitY, 0, sizeof(HitY));
	HitMissile = 0;
	HitFromExplosion = 0;
	NaturalDamage = 0;
	HitZ = 0;
	HitSourceItem = 0;
}
