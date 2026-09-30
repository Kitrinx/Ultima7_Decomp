/* Black Gate U7.EXE, overlay segment 224 (file offsets 0x065ba0 to 0x0671a3, 5635 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "u7npc.h"
#include "coord.h"
#include "daze.h"
#include "random.h"
#include "cheat.h"
#include "combwpn.h"
#include "death.h"
#include "makemojo.h"
#include "monsters.h"
#include "palctrl.h"
#include "slime.h"
#include "sounds.h"
#include "special.h"
#include "type.h"
#include "objref.h"
#include "npcref.h"
#include "search.h"
#include "voolook.h"
#include "wihh.h"
#include "weapons.h"
#include "ammo.h"
#include "armor.h"
#include "damage.h"

extern unsigned char far Item_getQuantity(objref *ref);
extern char far Item_getHitPoints(objref *ref);

/* Searches that answer yes or no. */
inline unsigned char Find(AreaSearch *search, objref container, int flags, int type, char quality, int frame)
{
	return FindItemInContainer(search, container, flags, type, quality, frame);
}
inline unsigned char Find(AreaSearch *search, Loc x, Loc y, int flags, int type, char quality, int frame)
{
	return FindItemInArea(search, x, y, flags, type, quality, frame);
}

struct ArmorRecord {
	int type;
	signed char armor;
	unsigned char unusedByte, immunities;
	char rest[5];
	unsigned char immuneTo(unsigned char kind) { return (immunities >> kind) & 1; }
};
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
};
struct ArmorRef {
	int index;
	ArmorRef() {}
	ArmorRef(int n) { index = n; }
	ArmorRecord *operator->() { return ArmorRecords.get(index); }
};
struct MonsterRef {
	int index;
	MonsterRef() {}
	MonsterRef(int n) { index = n; }
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
	unsigned char none() { return index == 0; }
};
extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int);
extern void far Item_setTemporary(objref *);
extern void far Item_setHitPoints(objref *, unsigned char);

extern unsigned char far Item_getCharges(objref *ref);
extern "C" void far HandleNpcHit(objref, NPCRef, char);

unsigned char far RollToWin(unsigned char attack, unsigned char defence)
{
	int roll = GenerateRandomIntegerInRange(30) + 1;

	if (roll == 1)
		return 0;
	if (roll == 30)
		return 1;
	return roll + attack - defence >= 15;
}

extern "C" unsigned char far RollToHit(NPCRef attacker, objref target, int bonus)
{
	if (!target.isNpc())
		return 1;
	if (target.multipart()) {
		target = GetStrangeMoverTarget(target);
	}
	int defence = NPCRef(target).combat();
	if (NPCRef(target).flag(NPC_PROTECTED))
		defence += 3;
	return RollToWin(attacker.combat() + bonus + 6, defence);
}

int far CountWeaponAmmo(objref npc, int weapon)
{
	objref ammo;
	return FindWeaponAmmo(npc, weapon, ammo);
}

int far FindWeaponAmmo(objref npc, int weaponNumber, objref far &ammo)
{
	WeaponRef weapon = weaponNumber;
	AreaSearch items;
	int count = 0;

	if (weapon.none()) {
		ammo = 0;
		return 99;
	}
	/* ammo -1: none needed; -2: the weapon's own charges; -3: thrown, counted in the pack;
	 * otherwise an ammunition type, found in the ammo slot */
	unsigned type = weapon->ammo;
	if (type == -1) {
		if (!Find(&items, npc, 1, weapon->type, 0xff, 0xff))
			Find(&items, npc, 0, weapon->type, 0xff, 0xff);
		ammo = items.current;
		return 99;
	} else if (type == -2) {
		Find(&items, npc, 1, weapon->type, 0xff, 0xff);
		while (items.found() && Item_getCharges(&items.current) == 0)
			FindItem(&items);
		if (items.found()) {
			ammo = items.current;
			return Item_getCharges(&items.current);
		}
		return 0;
	} else if (type == -3) {
		ammo = 0;
		if (Find(&items, npc, 2, weapon->type, 0xff, 0xff)) {
			ammo = items.current;
			count += Item_getQuantity(&items.current);
			while (FindItem(&items))
				count++;
		}
		if (Find(&items, npc, 1, weapon->type, 0xff, 0xff)) {
			if (!ammo.valid())
				ammo = items.current;
			count += Item_getQuantity(&items.current);
			while (FindItem(&items))
				count++;
		}
		return count;
	} else if (type < 32768) {
		ammo = GetItemInSlot(npc, 8);
		if (ammo.valid()) {
			count = Item_getQuantity(&ammo);
			if (ammo.type() != type) {
				AmmoRef kind = GetAmmoNumber(ammo);
				if (kind->family != type)
					count = 0;
			}
		}
		/* a triple crossbow fires three bolts */
		if (type == 723 /* bolt */ && weapon->projectile == 948)
			count /= 3;
		if (count == 0)
			ammo = 0;
		return count;
	}
	return 0;
}

unsigned far GetWeaponRange(objref npc, int weaponNumber)
{
	WeaponRef weapon = weaponNumber;

	if (weapon.none()) {
		MonsterRef monster = GetMonsterNumber(npc);
		return (unsigned char)monster->range;
	} else {
		WeaponRecord details;
		unsigned range;

		WeaponRecords.read(weapon.index, &details);
		switch ((unsigned char)details.uses) {
		case 2:
			range = Npc_getCombat(&npc) * 2;
			if (Npc_getStrength(&npc) * 2 < range)
				range = Npc_getStrength(&npc) * 2;
			if (range < details.range)
				range = details.range;
			return range;
		case 1:
			range = Npc_getCombat(&npc);
			if (Npc_getStrength(&npc) < range)
				range = Npc_getStrength(&npc);
			if (range < details.range)
				range = details.range;
			return range;
		case 0:
		case 3:
			return details.range;
		}
	}
}

int far DealDamage(unsigned char strength, int damage, int type, objref target, objref attacker)
{
	unsigned char kind = type;
	int total = 0;

	if (target.multipart()) {
		target = GetStrangeMoverTarget(target);
	}
	if (damage == 127) {           /* always lethal, no rolls */
		total = 127;
	} else {
		if (kind != 3 && strength > 0)
			total += RandomBetween(1, strength / 3);
		total += RandomBetween(1, damage);
	}
	if (target.isNpc()) {
		MonsterRef monster;
		int protection;

		monster = MonsterLookup.get(target.type());
		protection = (unsigned char)monster->armor;
		AreaSearch items;

		if (Find(&items, target, 1, -1, 0xff, 0xff)) {
			ArmorRef armor;

			do {
				if (GetItemInSlot(target, 3) != items.current) {
					armor = GetArmorNumber(items.current);
					protection += armor->armor;
					if (armor->immuneTo(kind)) {
						PlaySoundAtItem(5, target);
						HandleNpcHit(attacker, target, Item_getHitPoints(&target));
						return 0;
					}
				}
			} while (FindItem(&items));
		}
		if (damage != 127 && protection > 0 && kind != 3 && kind != 4 && kind != 5)
			total -= RandomBetween(1, protection);
	}
	if (total <= 0 && target.isNpc() && NPCRef(target).unconscious())
		total = GenerateRandomIntegerInRange(strength / 3) + 1;
	if (total <= 0) {
		ShowHarmlessHit(target);
		HandleNpcHit(attacker, target, Item_getHitPoints(&target));
		return 0;
	}
	int result = ReduceHealth(target, total, kind, attacker);
	return result > 0 ? result : -1;
}

void far ShowHarmlessHit(objref target)
{
	AreaSearch items;

	if (target.isNpc()) {
		NPCRef npc = target;
		npc.buffer()->typeFlags = npc.buffer()->typeFlags | 8;
	}
	PlaySoundAtItem(5, target);
}

int far ReduceHealth(objref target, int damage, int type, objref attacker)
{
	unsigned char kind = type;
	int hp = Item_getHitPoints(&target);
	if (hp == 0)
		hp = GetEffectiveHitPoints(target);
	int remaining = hp;
	int result = 0;
	MonsterRef monster;
	objref blood;
	MonsterRecord details;
	AreaSearch items;

	if (!target.isNpc()) {
		if (kind == 1 && target.type() == 704 /* powder keg */ && hp != 0) {
			BreakHitItem(target);
		} else if (remaining != 0 && kind != 3 && kind != 4) {
			if (remaining <= damage)
				BreakHitItem(target);
			else
				Item_setHitPoints(&target, remaining - damage);
		}
	} else {
		NPCRef npc = target;
		objref source = attacker;

		if (npc.flag(NPC_DEAD)) {
			HandleNpcHit(source, npc, hp);
			return 0;
		}
		monster = MonsterLookup.get(target.type());
		if (!monster.none())
			MonsterRecords.read(monster.index, &details);
		else
			FillFarBytes(&details, 25, 0);
		if (details.cantDie) {
			HandleNpcHit(source, npc, hp);
			return 0;
		}
		if (target == AvatarRef && PowerAvatar) {
			HandleNpcHit(source, npc, hp);
			return 0;
		}
		if ((details.immune >> kind) & 1) {
			HandleNpcHit(source, npc, hp);
			return 0;
		}
		if (damage < 127 && (details.vulnerable >> kind) & 1)
			damage <<= 1;
		if (target == AvatarRef && (kind == 3 || NPCRef(target).buffer()->strength / 3 < damage ||
									(NPCRef(target).buffer()->strength >> 2) > remaining))
			FlashDamagePalette();
		npc.buffer()->typeFlags = npc.buffer()->typeFlags | 8;
		/* plain wounds may bleed, unless the monster has no blood */
		if (kind == 0 && !(details.extraFlags & 0x40) && GenerateRandomIntegerInRange(10) < damage) {
			npc.buffer()->typeFlagsHigh = npc.buffer()->typeFlagsHigh | 4;
			int width = GetFootprintX(target.ptr()->asTypeFrame());
			int depth = GetFootprintY(target.ptr()->asTypeFrame());
			int size = gItemTypeInfo[target.type()].height;
			Coord x, y;
			x = Item_getX(target) - width - size + 2 + GenerateRandomIntegerInRange(size * 2 + width - 2);
			y = Item_getY(target) - depth - size + 2 + GenerateRandomIntegerInRange(size * 2 + depth - 2);
			if (!Item_getZ(&target)) {
				if (!Find(&items, x, y, 0, 912 /* blood */, 0xff, 0xff)) {
					if (CreateItem(&blood, TypeFrame(912 | GenerateRandomIntegerInRange(4) << 10 & 0x7c00 |
														((char)GenerateRandomIntegerInRange(2) ? 0x8000 : 0)), x, y, 0))
						Item_setTemporary(&blood);
				} else {
					items.current.ptr()->asTypeFrame().setFlipFrame(GenerateRandomIntegerInRange(4) +
						(GenerateRandomIntegerInRange(2) ? 0x8000 : 0));
				}
			}
		}
		remaining -= damage;
		if (details.splits && remaining > 0 && !((details.vulnerable >> kind) & 1)) {
			SplitCreature(npc);
		}
		if (remaining < -(npc.buffer()->strength / 3)) {
			Item_setHitPoints(&npc, remaining);
			result = KillNpc(target, attacker);
		} else if (remaining <= 0 && hp > 0) {
			result = KnockOut(npc, remaining);
			HandleNpcHit(source, npc, hp);
		} else {
			Item_setHitPoints(&npc, remaining);
			HandleNpcHit(source, npc, hp);
		}
	}
	return result;
}
