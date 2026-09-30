#ifndef WEAPONS_H
#define WEAPONS_H

#include "lowlevel.h"
#include "makemojo.h"
#include "voolook.h"

/* uses */
#define USES_MELEE          0
#define USES_POOR_THROWN    1
#define USES_GOOD_THROWN    2
#define USES_MISSILE        3

/* One WEAPONS.DAT record. */
struct WeaponRecord {
	int type;
	int ammo;                       /* ammo family, or -1 none, -2 thrown itself, -3 fires itself */
	int projectile;
	signed char damage;
	unsigned lucky : 1, explodes : 1, passesBlockers : 1, consumed : 1, damageType : 4;
	unsigned autoHit : 1, uses : 2, range : 5;
	unsigned returns : 1, homing : 1, missileSpeed : 2, frameStep : 4;
	unsigned meleeReadyFrame : 1, meleeStrikeFrame : 1, rangedReadyFrame : 1, rangedStrikeFrame : 1,
		unusedFlag : 1, speed : 3;
	unsigned sleep : 1, charm : 1, curse : 1, poison : 1, paralyze : 1, drainMana : 1, drainHealth : 1,
		noDamage : 1;
	unsigned char unusedByte;
	int usecode;
	char sound;
	char unusedTail[5];
};

/* Weapon records: 21-byte records in voodoo memory, the last one read kept in current. */
struct WeaponTable {
	long base;
	char current[21];
	long cached, count;
	WeaponTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	WeaponRecord *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 21, 21);
			cached = n;
		}
		return (WeaponRecord *)current;
	}
	void read(long n, WeaponRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 21, 21);
	}
};

extern char WeaponsFileName[];
extern WeaponTable WeaponRecords;
extern unsigned char WeaponCount;

extern ShapeLookup WeaponLookup;

#endif
