#ifndef MONSTERS_H
#define MONSTERS_H

#include "lowlevel.h"
#include "makemojo.h"

struct ShapeLookup;

/* One MONSTERS.DAT record. */
struct MonsterRecord {
	int16_t type;
	uint16_t sleepSafe : 1, charmSafe : 1, strength : 6;
	uint16_t curseSafe : 1, paralysisSafe : 1, dexterity : 6;
	uint16_t poisonSafe : 1, intFlag : 1, intelligence : 6;
	uint16_t alignment : 2, combat : 6;
	uint16_t splits : 1, cantDie : 1, powerSafe : 1, deathSafe : 1, armor : 4;
	uint8_t unusedByte;
	uint16_t range : 4, damage : 4;
	uint16_t fly : 1, swim : 1, walk : 1, ethereal : 1, noBody : 1, gazerFlag : 1, startInvisible : 1,
		seeInvisible : 1;
	uint8_t vulnerable, immune;
	uint8_t extraFlags;       /* 0x20 no barks or loot bag, 0x40 no blood */
	uint16_t category : 3, unusedBits : 5;
	uint8_t equipment;
	char unusedBytes[2];
	int16_t sound;
	char unusedTail[6];
	int16_t attackSound() { return sound; }
};

/* Monster records: 25-byte records in voodoo memory, the last one read kept in current. */
struct MonsterTable {
	int32_t base;
	char current[25];
	int32_t cached, count;
	MonsterTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	MonsterRecord *get(int32_t n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 25, 25);
			cached = n;
		}
		return (MonsterRecord *)current;
	}
	void read(int32_t n, MonsterRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 25, 25);
	}
};

extern MonsterTable MonsterRecords;

extern char MonstersFileName[];
extern uint8_t MonsterCount;
extern ShapeLookup MonsterLookup;

#endif
