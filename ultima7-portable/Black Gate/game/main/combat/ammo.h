#ifndef AMMO_H
#define AMMO_H

#include "lowlevel.h"
#include "makemojo.h"

struct ShapeLookup;

/* One AMMO.DAT record. */
struct AmmoRecord {
	int16_t type;
	int16_t family;
	int16_t projectile;
	int8_t damage;
	uint8_t lucky : 1, autoHit : 1, returns : 1, passesBlockers : 1, removeOnStop : 1, keepOnStop : 1, explodes : 1,
		unusedFlag : 1;
	uint8_t unusedByte;
	uint8_t unusedBits : 4, damageType : 4;
	uint8_t sleep : 1, charm : 1, curse : 1, poison : 1, paralyze : 1, drainMana : 1, drainHealth : 1,
		noDamage : 1;
	char unusedTail[2];
};
static_assert(sizeof(AmmoRecord) == 13, "AmmoRecord is read from a 13-byte file record");

/* Ammunition records: 13-byte records in voodoo memory, the last one read kept in current. */
struct AmmoTable {
	int32_t base;
	char current[13];
	int32_t cached, count;
	AmmoTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	AmmoRecord *get(int32_t n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 13, 13);
			cached = n;
		}
		return (AmmoRecord *)current;
	}
	void read(int32_t n, AmmoRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 13, 13);
	}
};

extern char AmmoFileName[];
extern AmmoTable AmmoRecords;
extern uint8_t AmmoCount;
extern ShapeLookup AmmoLookup;

#endif
