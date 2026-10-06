#ifndef VOOLOOK_H
#define VOOLOOK_H

#include "lowlevel.h"
#include "makemojo.h"
#include "itemrec.h"

/* A table of 1024 words in voodoo memory, one per type, with a checksum to catch stray writes. */
struct ShapeLookup {
	char tag;
	long addr;
	long sum;
	void computeChecksum();
	void verifyChecksum(int, int);
	void create(int fill, char tag);
	unsigned get(unsigned type)
	{
		CheckMojoBounds(1024L, (long) type);
		return PeekWord(addr + type * 2);
	}
	void set(unsigned type, int n)
	{
		CheckMojoBounds(1024L, (unsigned long) type);
		PokeWord(addr + type * 2, n);
		computeChecksum();
	}
};

extern ShapeLookup WeaponLookup;
extern ShapeLookup ArmorLookup;
extern ShapeLookup MonsterLookup;
extern ShapeLookup AmmoLookup;
extern ShapeLookup ReadyLookup;

/* The record numbers of an item's type in the weapon, armour, ammunition and monster tables. */
inline int GetWeaponNumber(objref item) { return WeaponLookup.get(item.type()); }
inline int GetArmorNumber(objref item) { return ArmorLookup.get(item.type()); }
inline int GetAmmoNumber(objref item) { return AmmoLookup.get(item.type()); }
inline int GetMonsterNumber(objref item) { return MonsterLookup.get(item.type()); }

extern int LoadedWeaponCount;
extern int LoadedAmmoCount;
extern int LoadedArmorCount;
extern int LoadedMonsterCount;
extern int LoadedReadyCount;

void LoadItemDataFiles(void);
void VerifyShapeLookups(int unused1, int unused2);

#endif
