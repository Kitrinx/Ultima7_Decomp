#ifndef READY_H
#define READY_H

#include "lowlevel.h"
#include "makemojo.h"
#include "voolook.h"

/* One READY.DAT record: where a type is worn or held. */
struct ReadyRecord {
	int16_t type;
	uint8_t spell : 1, unusedBits : 2, slot : 5;
	char unusedTail[6];
};
static_assert(sizeof(ReadyRecord) == 9, "ReadyRecord is read from a 9-byte file record");

/* Ready records: 9-byte records in voodoo memory, the last one read kept in current. */
struct ReadyTable {
	int32_t base;
	char current[9];
	int32_t cached, count;
	ReadyTable() { base = 0; cached = -1; count = 0; }
	void load(char *name);
	ReadyRecord *get(int32_t n)
	{
		CheckMojoBounds(count, n);
		if (n != cached) {
			CopyLinearToFar(current, base + n * 9, 9);
			cached = n;
		}
		return (ReadyRecord *)current;
	}
	void read(int32_t n, ReadyRecord *record)
	{
		CheckMojoBounds(count, n);
		CopyLinearToFar(record, base + n * 9, 9);
	}
};

extern char ReadyFileName[];
extern ReadyTable ReadyRecords;
extern uint8_t ReadyCount;

extern ShapeLookup ReadyLookup;

#endif
