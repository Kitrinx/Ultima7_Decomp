/* Black Gate U7.EXE, overlay segment 268 (file offsets 0x07f550 to 0x07fbd7, 1671 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: voolook.c */
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

struct ArmorRecord { unsigned type; char unusedFields[8]; };

int LoadedWeaponCount;
int LoadedAmmoCount;
int LoadedArmorCount;
int LoadedMonsterCount;
int LoadedReadyCount;

void ShapeLookup::computeChecksum()
{
	long end;
	long address;

	sum = 0;
	end = addr + 2048;
	for (address = addr; address < end; address += 4)
		sum += PeekLong(address);
}

void ShapeLookup::verifyChecksum(int, int)
{
	long old;

	old = sum;
	computeChecksum();
	if (sum != old)
		AssertFail(__FILE__, 88);
}

void ShapeLookup::create(int fill, char newTag)
{
	int i;
	long address;

	tag = newTag;
	addr = AllocateVoodooMemory(&VoodooXmsBlock, 2048L);
	if (addr == 0)
		ReportOutOfVoodooMemory();
	for (i = 0, address = addr; i < 1024; i++, address += 2)
		PokeWord(address, fill);
	computeChecksum();
}

/* Opens the item data files and builds the per-type lookups into them. */
void LoadItemDataFiles(void)
{
	int i;
	unsigned type;

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
void VerifyShapeLookups(int unused1, int unused2)
{
	WeaponLookup.verifyChecksum(unused1, unused2);
	ArmorLookup.verifyChecksum(unused1, unused2);
	MonsterLookup.verifyChecksum(unused1, unused2);
	AmmoLookup.verifyChecksum(unused1, unused2);
	ReadyLookup.verifyChecksum(unused1, unused2);
}
