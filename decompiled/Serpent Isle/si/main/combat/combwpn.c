/* Serpent Isle SI.EXE, overlay segment 364 (file offsets 0x0b65b0 to 0x0b8127, 7031 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

/* path: combwpn.c */
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
#include "search.h"
#include "sprite.h"

struct WeaponRef {
	int index;
	WeaponRef(int n) { index = n; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
	unsigned char none() { return index == -1; }
};
struct AmmoRef {
	int index;
	AmmoRef(int n) { index = n; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
	unsigned char none() { return index == 0; }
	unsigned char damageType() { return AmmoRecords.get(index)->damageType; }
	unsigned char explodes() { return AmmoRecords.get(index)->explodes; }
	unsigned char sleep() { return AmmoRecords.get(index)->sleep; }
	unsigned char charm() { return AmmoRecords.get(index)->charm; }
	unsigned char curse() { return AmmoRecords.get(index)->curse; }
	unsigned char poison() { return AmmoRecords.get(index)->poison; }
	unsigned char paralyze() { return AmmoRecords.get(index)->paralyze; }
	unsigned char drainMana() { return AmmoRecords.get(index)->drainMana; }
	unsigned char drainHealth() { return AmmoRecords.get(index)->drainHealth; }
	unsigned char noDamage() { return AmmoRecords.get(index)->noDamage; }
};

objref HitTargetItem;
Coord HitX, HitY;
int HitMissile;
unsigned char HitFromExplosion;
int NaturalDamage, HitZ, HitSourceItem;

static void far ApplyHit(objref attacker, int weaponNumber, int ammoNumber);

unsigned char far DeleteMatchingInventoryItems(objref container, int type, char quality, int frame)
{
	AreaSearch search;
	unsigned char deleted = 0;
	for (FindItemInContainer(&search, container, 0, type, quality, frame);
		search.found();
		FindItemInContainer(&search, container, 0, type, quality, frame)) {
		deleted = 1;
		Item_delete(&search.current);
	}
	return deleted;
}

void far ApplyMonsterHit(objref attacker, objref target, char damage)
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

void far ApplyHitAtCoords(objref attacker, Coord x, Coord y, int z, int weapon, int ammo, int missile)
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

void far ApplyWeaponHit(objref attacker, objref target, int weapon, int ammo, int missile, unsigned char fromExplosion)
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

static void far ApplyHit(objref attacker, int weaponNumber, int ammoNumber)
{
	WeaponRef weapon = weaponNumber;
	AmmoRef ammo = ammoNumber;
	NPCRef self;
	unsigned char damageType, explodes;
	int event;
	unsigned char strength, intelligence;
	int result, resist;
	unsigned char sleep, charm, curse, poison, paralyze, drainMana, drainHealth, effect7;
	unsigned char alignment, targetAlignment;
	int damage;

	if (weaponNumber == 0 && ammoNumber == 0)
		return;
	resist = 16;
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
		extern int HitMissile;

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
			} else if (!effect7)
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
					if (drainMana) {
						if (npc.isAvatar())
							npc.buffer()->mana = npc.buffer()->mana & 0xe0;
						else {
							Npc_setNoCastFlag(&npc);
							unsigned char deleted = DeleteMatchingInventoryItems(npc, 397, -1, 255);
							deleted = DeleteMatchingInventoryItems(npc, 398, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 676, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 731, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 540, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 280, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 281, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 287, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 339, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 399, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 408, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 424, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 443, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 527, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 566, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 621, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 639, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 807, -1, 255) || deleted;
							deleted = DeleteMatchingInventoryItems(npc, 856, -1, 255) || deleted;
							if (deleted) {
								ClearAllBarks(npc);
								BarkLine(&npc, 155 + GenerateRandomIntegerInRange(3), 5);
							}
						}
					}
					if (drainHealth
						&& (unsigned char)(gItemTypeInfo[HitTargetItem.type()].typeClass == TYPE_CLASS_HUMAN))
						npc.buffer()->hitPoints = RandomBetween(0, npc.buffer()->hitPoints);
				}
				if (effect7) {
					int script = 0;
					switch (objref(HitSourceItem).type()) {
					case 591:
						script = 2013;
						break;
					case 568:
						script = 2017;
						break;
					}
					if (script)
						RunUsable(4, HitTargetItem, script);
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
					AwardExperience(self, (long)result);
			}
		}
		if (!weapon.none() && event != 0)
			RunUsable(4, HitTargetItem, event);
	}
}

void far BreakHitItem(objref ref)
{
	int type = ref.type();
	if (type == 704 /* powder keg */)
		ExplodePowderKeg(ref);
	else if (!PowderKegExploding)
		RunUsable(4, ref, 0x626);
}
