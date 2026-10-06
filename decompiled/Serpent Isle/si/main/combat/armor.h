#ifndef ARMOR_H
#define ARMOR_H

#include "lowlevel.h"
#include "makemojo.h"

struct ArmorRecord;
struct ShapeLookup;

/* Armor records: 10-byte records in voodoo memory, the last one read kept in current. */
struct ArmorTable {
	long base;
	char current[10];
	long cached, count;
	ArmorTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	ArmorRecord *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 10, 10);
			cached = n;
		}
		return (ArmorRecord *)current;
	}
	void read(long n, ArmorRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 10, 10);
	}
};

extern char ArmorFileName[];
extern ArmorTable ArmorRecords;
extern unsigned char ArmorCount;
extern ShapeLookup ArmorLookup;

#endif
