/* Serpent Isle SI.EXE, overlay segment 360 (file offsets 0x0b4610 to 0x0b54e8, 3800 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "item.h"
#include "voolook.h"
#include "u7npc.h"
#include "debug.h"
#include "makemojo.h"
#include "random.h"
#include "bodies.h"
#include "cast.h"
#include "equip.h"
#include "search.h"
#include "combat.h"
#include "wihh.h"
#include "type.h"
#include "ready.h"
#include "weapons.h"
#include "monsters.h"
#include "npcref.h"
#include "text.h"

struct EquipmentEntry {
	int type;
	unsigned char chance, count;
	unsigned char rest[2];
};
struct EquipmentList { EquipmentEntry entries[10]; };
struct WeaponRef {
	int index;
	WeaponRef() {}
	void operator=(int n) { index = n; }
	unsigned char isNull() { return index == 0; }
	WeaponRecord *operator->() { return WeaponRecords.get(index); }
};
struct MonsterRef {
	int index;
	MonsterRef(int n) { index = n; }
	operator int() { return index; }
};
#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define NPC(ref) GetNpcBufferForIbo(ref)

void far RandomizeCreature(objref ref, TypeFrame far &appearance)
{
	MonsterRef index = MonsterLookup.get(appearance.type());
	MonsterRecord record;
	MonsterRecords.read(index, &record);

	NPC(&ref)->strength =
		(unsigned char)(NPC(&ref)->strength & 0xe0) |
		(unsigned char)(record.strength < 8
		? GenerateRandomIntegerInRange(record.strength) + GenerateRandomIntegerInRange(record.strength) + 1
		: record.strength + GenerateRandomIntegerInRange(5) + GenerateRandomIntegerInRange(5) - 4);
	NPC(&ref)->dexterity = record.dexterity < 8
		? GenerateRandomIntegerInRange(record.dexterity) + GenerateRandomIntegerInRange(record.dexterity) + 1
		: record.dexterity + GenerateRandomIntegerInRange(5) + GenerateRandomIntegerInRange(5) - 4;
	NPC(&ref)->intelligence =
		(unsigned char)(record.intelligence < 8
		? GenerateRandomIntegerInRange(record.intelligence) + GenerateRandomIntegerInRange(record.intelligence) + 1
		: record.intelligence + GenerateRandomIntegerInRange(5) + GenerateRandomIntegerInRange(5) - 4) |
		(unsigned char)(NPC(&ref)->intelligence & 0xe0);
	NPC(&ref)->combat =
		(unsigned char)(record.combat < 8
		? GenerateRandomIntegerInRange(record.combat) + GenerateRandomIntegerInRange(record.combat) + 1
		: record.combat + GenerateRandomIntegerInRange(5) + GenerateRandomIntegerInRange(5) - 4) |
		(unsigned char)(NPC(&ref)->combat & 0xe0);
	NPC(&ref)->hitPoints = NPC(&ref)->strength & 0x1f;
	Npc_clearMetFlag(&ref);
	Npc_clearNoCastFlag(&ref);
	Npc_setTemperature(&ref, 0);
	Npc_clearFreezeFlag(&ref);
	Npc_setMagic(&ref, 0);
	Npc_clearZombieFlag(&ref);
	Item_setHitPoints(&ref, NPC(&ref)->strength & 0x1f);
	if (record.startInvisible)
		Item_setInvisible(&ref);
	Item_setTemporary(&ref);
	EquipCreature(ref, index);

	if (record.walk)
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags | 0x20;
	else
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags & ~0x20;
	if (record.swim)
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags | 0x40;
	else
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags & ~0x40;
	if (record.fly)
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags | 0x10;
	else
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags & ~0x10;
	if (record.ethereal) {
		if ((unsigned char)gItemTypeInfo[TYPE(ITEM(ref.off))].solid) {
			CheatPrintfWait(GetGameText(3, 238), Item_getNpcNumber(&ref));
			NPC(&ref)->typeFlags = NPC(&ref)->typeFlags | 0x10;
		} else
			NPC(&ref)->typeFlags = NPC(&ref)->typeFlags | 0x80;
	} else
		NPC(&ref)->typeFlags = NPC(&ref)->typeFlags & ~0x80;
}

void far EquipCreatureByType(objref *ref)
{
	int index;
	unsigned type = TYPE(objref(ref->off).ptr());

	CheckMojoBounds(1024L, (long)type);
	index = PeekWord(MonsterLookup.addr + type * 2);
	EquipCreature(objref(ref->off), index);
}

unsigned char far EquipCreature(objref ref, int monsterIndex)
{
	int type = monsterIndex;
	unsigned equipment;
	objref item, other;
	unsigned char slot;
	WeaponRef weapon;
	int ammoType;
	int remaining;
	unsigned char madeAny = 0;
	unsigned char handled;
	unsigned char needsContainer = 0;
	EquipmentList list;
	int i;

	equipment = MonsterRecords.get(type)->equipment;
	if (equipment == 0)
		return 0;
	EquipRecords.read(equipment, &list);
	for (i = 0; i < 10; i++) {
		if (list.entries[i].type == 0)
			break;
		if (list.entries[i].type != -1 &&
			GenerateRandomIntegerInRange(100) < list.entries[i].chance) {
			if (CreateItemInContainer(&item, list.entries[i].type, ref)) {
				Item_setTemporary(&item);
				Item_setOkayToTake(&item);
				madeAny = 1;
				handled = 0;
				weapon = WeaponLookup.get(list.entries[i].type);
				if (!weapon.isNull()) {
					ammoType = weapon->ammo;
					if (ammoType == -2) {
						Item_setQuality(&item, list.entries[i].count);
						handled = 1;
					} else if (ammoType != -1 && ammoType != -3) {
						if (CreateItemInContainer(&other, ammoType, ref)) {
							Item_setTemporary(&other);
							Item_setOkayToTake(&other);
							Item_setQuantity(other,
								GenerateRandomIntegerInRange(10) + GenerateRandomIntegerInRange(10) + 2, 0);
						}
					}
				}
				if (!handled) {
					if ((unsigned char)Item_hasQuantity(&item)) {
						Item_setQuantity(item, GenerateRandomIntegerInRange(list.entries[i].count) + 1, 0);
					} else {
						if (list.entries[i].type == 377 /* food item */) {
							Item_setFrame(&item, GetMeatFrame(GetItemType(ref)));
						}
						for (remaining = list.entries[i].count - 1; remaining > 0; remaining--) {
							if (CreateItemInContainer(&item, list.entries[i].type, ref)) {
								Item_setTemporary(&item);
								Item_setOkayToTake(&item);
								if (list.entries[i].type == 377 /* food item */)
									Item_setFrame(&item, GetMeatFrame(GetItemType(ref)));
							} else {
								break;
							}
						}
					}
				}
				slot = ReadyRecords.get(ReadyLookup.get(list.entries[i].type))->slot;
				if (slot) {
					other = objref(GetItemInSlot(ref, slot));
					if (!other.valid())
						EquipItem(item, ref, slot, 0);
					else if (!(unsigned char)ReadyRecords.get(ReadyLookup.get(list.entries[i].type))->spell)
						needsContainer = 1;
				} else if (!(unsigned char)ReadyRecords.get(ReadyLookup.get(list.entries[i].type))->spell &&
					list.entries[i].type != 377 /* food item */) {
					needsContainer = 1;
				}
			}
		}
	}
	if (!madeAny)
		return 0;
	SelectWeapon(ref, 0, 0);
	if (needsContainer && !(MonsterRecords.get(type)->extraFlags & 0x20)) {
		int containerType;
		char quality = 0;
		AreaSearch items;
		switch (GenerateRandomIntegerInRange(5)) {
		case 0: containerType = 800; break;
		case 1: containerType = 522; quality = 0; break;
		case 2: containerType = 522; quality = -1; break;
		case 3: containerType = 801; break;
		case 4: containerType = 802; break;
		}
		if (CreateItemInContainer(&item, containerType, ref)) {
			Item_setQuality(&item, quality);
			Item_setTemporary(&item);
			Item_setOkayToTake(&item);
			FindItemInContainer(&items, ref, 0x102, -1, 0xff, 0xff);
			while (items.current.valid()) {
				if (items.current == item ||
					(unsigned char)ReadyRecords.get(ReadyLookup.get(items.current.ptr()->typeFrame & 0x3ff))->spell)
					FindItem(&items);
				else {
					Item_moveIntoContainer(&items.current, item);
					FindItemInContainer(&items, ref, 0x102, -1, 0xff, 0xff);
				}
			}
		}
	}
	return 1;
}
