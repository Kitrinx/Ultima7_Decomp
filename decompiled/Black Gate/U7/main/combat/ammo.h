#ifndef AMMO_H
#define AMMO_H

#include "lowlevel.h"
#include "makemojo.h"

struct ShapeLookup;

/* One AMMO.DAT record. */
struct AmmoRecord {
	int type;
	int family;
	int projectile;
	signed char damage;
	unsigned lucky : 1, autoHit : 1, returns : 1, passesBlockers : 1, removeOnStop : 1, keepOnStop : 1, explodes : 1,
		unusedFlag : 1;
	unsigned char unusedByte;
	unsigned unusedBits : 4, damageType : 4;
	unsigned sleep : 1, charm : 1, curse : 1, poison : 1, paralyze : 1, drainMana : 1, drainHealth : 1,
		noDamage : 1;
	char unusedTail[2];
};

/* Ammunition records: 13-byte records in voodoo memory, the last one read kept in current. */
struct AmmoTable {
	long base;
	char current[13];
	long cached, count;
	AmmoTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	AmmoRecord *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 13, 13);
			cached = n;
		}
		return (AmmoRecord *)current;
	}
	void read(long n, AmmoRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 13, 13);
	}
};

extern char AmmoFileName[];
extern AmmoTable AmmoRecords;
extern unsigned char AmmoCount;
extern ShapeLookup AmmoLookup;

#endif
