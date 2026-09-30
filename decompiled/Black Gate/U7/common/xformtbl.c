/* Black Gate U7.EXE, resident segment 97 (file offsets 0x035db6 to 0x035f91, 475 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "lowlevel.h"
#include "vooalloc.h"
#include "flex.h"
#include "oops.h"

void MoveBufferToVoodoo(long *block, void *buffer, int size);

/* Load count 256-byte records from a flex into far memory, then blanks empty ones, after a 256-byte
 * table that maps 254 - i to record i + 1. */
void LoadXformTables(long *table, char *name, int count, int blanks)
{
	int size;
	char *buf;
	char *p;
	int i, j;

	size = (count + blanks + 1) << 8;
	buf = new char[size];
	if (buf == 0)
		ReportOutOfNearMemory();
	p = buf;
	for (i = 0; i < 256; i++)
		p[i] = 255;
	for (i = 0; i < count; i++)
		*(p + 254 - i) = i + 1;
	p += 256;

	Flex file;
	file.openOrFail(name);
	for (i = 0; i < count + blanks; i++) {
		for (j = 0; j < 256; j++)
			p[j] = 0;
		if (i < count)
			file.readRecord(i, p, 0);
		p += 256;
	}
	file.close();
	MoveBufferToVoodoo(table, buf, size);
}

void MoveBufferToVoodoo(long *block, void *buffer, int size)
{
	*block = AllocateVoodooMemory(&VoodooXmsBlock, (long) size);
	CopyFarToLinear(*block, (void far *) buffer, (long) size);
	delete buffer;
}

void SetXformEntries(long *table, unsigned char *indices, char value)
{
	for (; *indices != 255; indices++)
		PokeByte(*table + *indices, value);
}

void SetXformEntry(long *table, int index, char value)
{
	PokeByte(*table + index, value);
}

void SetXformTableByte(long *table, int record, int index, char value)
{
	PokeByte(*table + (record << 8) + index, value);
}
