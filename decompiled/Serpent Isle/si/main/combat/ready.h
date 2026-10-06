#ifndef READY_H
#define READY_H

#include "lowlevel.h"
#include "makemojo.h"
#include "voolook.h"

/* One READY.DAT record: where a type is worn or held. */
struct ReadyRecord {
	int type;
	unsigned spell : 1, unusedBits : 2, slot : 5;
	char unusedTail[6];
};

/* Ready records: 9-byte records in voodoo memory, the last one read kept in current. */
struct ReadyTable {
	long base;
	char current[9];
	long cached, count;
	ReadyTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	ReadyRecord *get(long n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 9, 9);
			cached = n;
		}
		return (ReadyRecord *)current;
	}
	void read(long n, ReadyRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 9, 9);
	}
};

extern char ReadyFileName[];
extern ReadyTable ReadyRecords;
extern unsigned char ReadyCount;

extern ShapeLookup ReadyLookup;

#endif
