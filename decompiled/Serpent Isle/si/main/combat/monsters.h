#ifndef MONSTERS_H
#define MONSTERS_H

#include "lowlevel.h"
#include "makemojo.h"

struct ShapeLookup;

/* One MONSTERS.DAT record. */
struct MonsterRecord {
	int type;
	unsigned sleepSafe : 1, charmSafe : 1, strength : 6;
	unsigned curseSafe : 1, paralysisSafe : 1, dexterity : 6;
	unsigned poisonSafe : 1, intFlag : 1, intelligence : 6;
	unsigned alignment : 2, combat : 6;
	unsigned splits : 1, cantDie : 1, powerSafe : 1, deathSafe : 1, armor : 4;
	unsigned char unusedByte;
	unsigned range : 4, damage : 4;
	unsigned fly : 1, swim : 1, walk : 1, ethereal : 1, noBody : 1, gazerFlag : 1, startInvisible : 1,
		seeInvisible : 1;
	unsigned char vulnerable, immune;
	unsigned char extraFlags;       /* 0x20 no barks or loot bag, 0x40 no blood */
	unsigned category : 3, unusedBits : 5;
	unsigned char equipment;
	char unusedBytes[2];
	int sound;
	char unusedTail[6];
	int attackSound() { return sound; }
};

/* Monster records: 25-byte records in voodoo memory, the last one read kept in current. */
struct MonsterTable {
	long base;
	char current[25];
	long cached, count;
	MonsterTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	MonsterRecord *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 25, 25);
			cached = n;
		}
		return (MonsterRecord *)current;
	}
	void read(long n, MonsterRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 25, 25);
	}
};

extern MonsterTable MonsterRecords;

extern char MonstersFileName[];
extern unsigned char MonsterCount;
extern ShapeLookup MonsterLookup;

#endif
