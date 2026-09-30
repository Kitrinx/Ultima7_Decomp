/* Black Gate U7.EXE, resident segment 97 (file offsets 0x035db6 to 0x035f91, 475 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "flex.h"
#include "oops.h"

void MoveBufferToVoodoo(int32_t *block, void *buffer, int16_t size);

/* Load count 256-byte records from a flex into far memory, then blanks empty ones, after a 256-byte
 * table that maps 254 - i to record i + 1. */
void LoadXformTables(int32_t *table, char *name, int16_t count, int16_t blanks)
{
	int16_t size;
	char *buf;
	char *p;
	int16_t i, j;

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

void MoveBufferToVoodoo(int32_t *block, void *buffer, int16_t size)
{
	*block = AllocateVoodooMemory(&VoodooXmsBlock, (int32_t) size);
	CopyFarToLinear(*block, (void *) buffer, (int32_t) size);
	delete buffer;
}

void SetXformEntries(int32_t *table, uint8_t *indices, int8_t value)
{
	for (; *indices != 255; indices++)
		PokeByte(*table + *indices, value);
}

void SetXformEntry(int32_t *table, int16_t index, int8_t value)
{
	PokeByte(*table + index, value);
}

void SetXformTableByte(int32_t *table, int16_t record, int16_t index, int8_t value)
{
	PokeByte(*table + (record << 8) + index, value);
}
