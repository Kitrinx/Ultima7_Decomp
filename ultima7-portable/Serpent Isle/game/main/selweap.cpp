/* Serpent Isle SI.EXE, overlay segment 238 (file offsets 0x067550 to 0x0687ef, 4767 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "itemrec.h"
#include "u7npc.h"
#include "equip.h"
#include "wihh.h"
#include "random.h"
#include "makemojo.h"
#include "monsters.h"
#include "search.h"
#include "combat.h"
#include "type.h"
#include "objref.h"
#include "damage.h"
#include "voolook.h"
#include "ready.h"
#include "weapons.h"
#include "ammo.h"
#include "armor.h"

inline uint16_t WeaponRange(objref &who, int16_t weapon) { return GetWeaponRange(who, weapon); }

extern uint8_t Item_moveIntoContainer(objref *ref, objref container);
extern uint8_t Item_getQuantity(objref *ref);

struct ArmorRecord { int16_t type; int8_t protection; char unusedField1[7]; };

struct WeaponRef {
	int16_t index;
	WeaponRef() {}
	WeaponRef(int16_t n) { index = n; }
	void operator=(int16_t n) { index = n; }
	uint8_t empty() { return index == 0; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
};

struct MonsterRef {
	int16_t index;
	MonsterRef() {}
	void operator=(int16_t n) { index = n; }
};

struct AmmoRef {
	int16_t index;
	AmmoRef() {}
	AmmoRef(int16_t n) { index = n; }
	void operator=(int16_t n) { index = n; }
	uint8_t empty() { return index == 0; }
	AmmoRecord *operator->() { return AmmoRecords.get(index); }
};

struct ArmorRef {
	int16_t index;
	ArmorRef(int16_t n) { index = n; }
	uint8_t empty() { return index == 0; }
	ArmorRecord *operator->() { return ArmorRecords.get(index); }
};

extern uint8_t Item_getCharges(objref *ref);

/* Scores a weapon for who (held, or who's own attack when held is empty); -1 when unusable. */
int16_t RateWeapon(objref who, objref held, int16_t minimum)
{
	WeaponRef weaponIndex;
	MonsterRef monsterIndex;
	AmmoRef ammoIndex;
	int16_t score;
	int16_t range;
	objref equipped;
	int16_t count = 0;
	WeaponRecord weaponData;
	MonsterRecord monsterData;
	AmmoRecord ammoData;
	AreaSearch items;

	if (!held.valid()) {
		weaponIndex = GetWeaponNumber(who);
		if (weaponIndex.empty()) {
			monsterIndex = GetMonsterNumber(who);
			MonsterRecords.read(monsterIndex.index, &monsterData);
			range = monsterData.range;
			if (range < minimum)
				score = -1;
			else
				score = 0;
			return score;
		}
	} else
		weaponIndex = GetWeaponNumber(held);
	if (weaponIndex.empty())
		return -1;
	WeaponRecords.read(weaponIndex.index, &weaponData);
	score = weaponData.damage;
	if (weaponData.explodes) {
		score = 29000;
	} else if (weaponData.usecode == 0x689) {
		score = 30000;
	} else {
		score += weaponData.sleep * 20 + weaponData.charm * 15
			+ weaponData.paralyze * 20 + weaponData.noDamage
			+ weaponData.curse + weaponData.poison * 10;
		range = WeaponRange(who, weaponIndex.index);
		if (range < minimum)
			score = -1;
		else
			score += range;
	}
	if (weaponData.returns)
		return score;
	switch (weaponData.ammo) {
	case -1:
		goto done;
	case -2:
		count = Item_getCharges(&held);
		break;
	case -3:
		if (!weaponData.uses)
			return score;
		FindItemInContainer(&items, who, 0, held.type(), 255, 255);
		while (items.current.valid()) {
			++count;
			FindItem(&items);
		}
		break;
	default:
		equipped = objref(GetItemInSlot(who, 10));
		if (equipped.valid()) {
			count = Item_getQuantity(&equipped);
			if (equipped.type() == (uint16_t)(weaponData.ammo)) {
				count = Item_getQuantity(&equipped);
				break;
			}
			ammoIndex = GetAmmoNumber(equipped);
			if (!ammoIndex.empty()) {
				AmmoRecords.read(ammoIndex.index, &ammoData);
				if (ammoData.family == weaponData.ammo) {
					count = Item_getQuantity(&equipped);
					break;
				}
			}
		}
		if (FindItemInContainer(&items, who, 0, weaponData.ammo, 255, 255)) {
			count = Item_getQuantity(&items.current);
			while (FindItem(&items))
				count += Item_getQuantity(&items.current);
			break;
		}
		count = 0;
		FindItemInContainer(&items, who, 0, -1, 255, 255);
		while (items.current.valid()) {
			ammoIndex = GetAmmoNumber(items.current);
			if (!ammoIndex.empty()) {
				AmmoRecords.read(ammoIndex.index, &ammoData);
				if (ammoData.family == weaponData.ammo)
					count += Item_getQuantity(&items.current);
			}
			FindItem(&items);
		}
		break;
	}
	if (count < 1)
		return -1;
done:
	return score;
}

/* Readies who's best weapon, a shield when the hand is free, and the best ammunition for it. Slots: 1
 * the weapon hand, 0 the other hand, 10 ammunition; 20 is both hands. */
uint8_t SelectWeapon(objref who, uint8_t randomize, int16_t mode)
{
	objref best, current;
	int16_t bestScore;
	WeaponRef weapon;
	int16_t ammoType;
	AmmoRef ammunition;
	objref oldAmmo;
	uint8_t occupied;
	int16_t rating;
	AreaSearch items;

	occupied = 0;

	current = objref(GetItemInSlot(who, 1));
	if (current.valid() && IsInParty(&who)) {
		weapon.index = GetWeaponNumber(current);
		if (CountWeaponAmmo(who, weapon.index))
			return 1;
		UnequipItem(current);
		current = objref(GetItemInSlot(who, 11));
		if (current.valid()) {
			bestScore = RateWeapon(who, current, 0);
			if (bestScore >= 0) {
				best = current;
				Item_moveIntoContainer(&current, who);
				goto selected;
			}
		}
		current = objref(GetItemInSlot(who, 17));
		if (current.valid()) {
			bestScore = RateWeapon(who, current, 0);
			if (bestScore >= 0) {
				best = current;
				Item_moveIntoContainer(&current, who);
				goto selected;
			}
		}
		current.off = 0;
	}
	if (current.valid())
		UnequipItem(current);
	if (IsInParty(&who)) {
		current = objref(GetItemInSlot(who, 0));
		if (current.valid() && (uint8_t)gItemTypeInfo[current.type()].light)
			occupied = 1;
	}
	best.off = 0;
	bestScore = RateWeapon(who, best, mode);
	for (FindItemInContainer(&items, who, 0, -1, 255, 255); items.current.valid(); FindItem(&items)) {
		if (occupied == 0 || (uint8_t)ReadyRecords.get(ReadyLookup.get(items.current.type()))->slot != 20) {
			if (items.current.type() == 704)    /* powder keg */
				rating = -1;
			else
				rating = RateWeapon(who, items.current, mode);
			if (randomize && rating >= 0)
				rating = GenerateRandomIntegerInRange(10000) + 1;
			if (rating > bestScore || (rating == bestScore && !best.valid())) {
				bestScore = rating;
				best = items.current;
			}
		}
	}
selected:
	if (!best.valid())
		return 0;
	if ((uint8_t)ReadyRecords.get(ReadyLookup.get(best.type()))->slot == 20) {
		current = objref(GetItemInSlot(who, 0));
		if (current.valid())
			UnequipItem(current);
		EquipItem(best, who, 20, 0);
	} else {
		EquipItem(best, who, 1, 0);
		current = objref(GetItemInSlot(who, 0));
		if (!current.valid()) {
			objref shield;
			shield.off = 0;
			bestScore = 0;
			for (FindItemInContainer(&items, who, 2, -1, 255, 255); items.current.valid(); FindItem(&items)) {
				uint8_t slot = ReadyRecords.get(ReadyLookup.get(items.current.type()))->slot;
				if (slot == 1 || slot == 0) {
					ArmorRef protection = GetArmorNumber(items.current);
					if (protection.empty() == 0 && protection->protection > bestScore) {
						shield = items.current;
						bestScore = protection->protection;
					}
				}
			}
			if (shield.valid())
				EquipItem(shield, who, 0, 0);
		}
	}
	weapon.index = GetWeaponNumber(best);
	ammoType = weapon->ammo;
	switch (ammoType) {
	case -3:
	case -2:
	case -1:
		break;
	default:
		best = objref(GetItemInSlot(who, 10));
		if (best.type() == (uint16_t)ammoType)
			return 1;
		ammunition.index = GetAmmoNumber(best);
		if (!ammunition.empty() && ammunition->family == ammoType)
			return 1;
		best.off = 0;
		bestScore = -1;
		for (FindItemInContainer(&items, who, 0, -1, 255, 255); items.current.valid(); FindItem(&items)) {
			if (items.current.type() == (uint16_t)ammoType)
				rating = 0;
			else {
				ammunition.index = GetAmmoNumber(items.current);
				if (ammunition.empty())
					rating = -1;
				else {
					AmmoRecord data;
					AmmoRecords.read(ammunition.index, &data);
					if (data.family != ammoType)
						rating = -1;
					else {
						rating = data.damage;
						if (data.lucky)
							++rating;
						if (data.damageType)
							++rating;
						if (data.sleep || data.charm || data.paralyze || data.noDamage)
							rating += 6;
						if (data.curse || data.poison || data.drainMana || data.drainHealth)
							++rating;
						if (data.autoHit)
							rating *= 2;
						if (data.passesBlockers)
							rating *= 2;
						if (Item_getQuantity(&items.current) < 5)
							rating /= 2;
					}
				}
			}
			if (rating > bestScore) {
				bestScore = rating;
				best = items.current;
			}
		}
		if (best.valid()) {
			oldAmmo = objref(GetItemInSlot(who, 10));
			if (oldAmmo.valid())
				UnequipItem(oldAmmo);
			EquipItem(best, who, 10, 0);
		}
	}
	return 1;
}
