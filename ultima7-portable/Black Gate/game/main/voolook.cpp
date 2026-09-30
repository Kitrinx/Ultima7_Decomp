/* Black Gate U7.EXE, overlay segment 268 (file offsets 0x07f550 to 0x07fbd7, 1671 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: voolook.c */
#include "u7port.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "init.h"
#include "makemojo.h"
#include "oops.h"
#include "easyfile.h"
#include "ammo.h"
#include "armor.h"
#include "equip.h"
#include "monsters.h"
#include "ready.h"
#include "weapons.h"
#include "debug.h"
#include "voolook.h"

struct ArmorRecord { uint16_t type; char unusedFields[8]; };

int16_t LoadedWeaponCount;
int16_t LoadedAmmoCount;
int16_t LoadedArmorCount;
int16_t LoadedMonsterCount;
int16_t LoadedReadyCount;

void ShapeLookup::computeChecksum()
{
	int32_t end;
	int32_t address;

	sum = 0;
	end = addr + 2048;
	for (address = addr; address < end; address += 4)
		sum += PeekLong(address);
}

void ShapeLookup::verifyChecksum(int16_t, int16_t)
{
	int32_t old;

	old = sum;
	computeChecksum();
	if (sum != old)
		AssertFail(__FILE__, 88);
}

void ShapeLookup::create(int16_t fill, int8_t newTag)
{
	int16_t i;
	int32_t address;

	tag = newTag;
	addr = AllocateVoodooMemory(&VoodooXmsBlock, INT32_C(2048));
	if (addr == 0)
		ReportOutOfVoodooMemory();
	for (i = 0, address = addr; i < 1024; i++, address += 2)
		PokeWord(address, fill);
	computeChecksum();
}

/* Opens the item data files and builds the per-type lookups into them. */
void LoadItemDataFiles(void)
{
	int16_t i;
	uint16_t type;

	WeaponRecords.load(BuildPath(StaticPath, WeaponsFileName, 0));
	ArmorRecords.load(BuildPath(StaticPath, ArmorFileName, 0));
	MonsterRecords.load(BuildPath(StaticPath, MonstersFileName, 0));
	AmmoRecords.load(BuildPath(StaticPath, AmmoFileName, 0));
	ReadyRecords.load(BuildPath(StaticPath, ReadyFileName, 0));
	EquipRecords.load(BuildPath(StaticPath, EquipFileName, 0));
	LoadedWeaponCount = WeaponCount;
	LoadedAmmoCount = AmmoCount;
	LoadedArmorCount = ArmorCount;
	LoadedMonsterCount = MonsterCount;
	LoadedReadyCount = ReadyCount;
	WeaponLookup.create(0, 'w');
	ArmorLookup.create(0, 'a');
	MonsterLookup.create(0, 'm');
	AmmoLookup.create(0, 'A');
	ReadyLookup.create(0, 'r');
	for (i = 1; i <= WeaponCount; i++)
		WeaponLookup.set(WeaponRecords.get(i)->type, i);
	for (i = 1; i <= ArmorCount; i++)
		ArmorLookup.set(ArmorRecords.get(i)->type, i);
	for (i = 1; i <= MonsterCount; i++) {
		type = MonsterRecords.get(i)->type;
		MonsterLookup.set(type, i);
	}
	for (i = 1; i <= AmmoCount; i++)
		AmmoLookup.set(AmmoRecords.get(i)->type, i);
	for (i = 1; i <= ReadyCount; i++) {
		type = ReadyRecords.get(i)->type;
		ReadyLookup.set(type, i);
	}
}

/* Asserts that no lookup changed since it was built; both arguments go unused. */
void VerifyShapeLookups(int16_t unused1, int16_t unused2)
{
	WeaponLookup.verifyChecksum(unused1, unused2);
	ArmorLookup.verifyChecksum(unused1, unused2);
	MonsterLookup.verifyChecksum(unused1, unused2);
	AmmoLookup.verifyChecksum(unused1, unused2);
	ReadyLookup.verifyChecksum(unused1, unused2);
}

extern "C" void ResetVoolookGlobals(void)
{
	LoadedWeaponCount = 0;
	LoadedAmmoCount = 0;
	LoadedArmorCount = 0;
	LoadedMonsterCount = 0;
	LoadedReadyCount = 0;
}
