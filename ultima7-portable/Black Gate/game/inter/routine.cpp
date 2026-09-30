/* Black Gate U7.EXE, overlay segment 323 (file offsets 0x0967d0 to 0x0971e3, 2579 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <string.h>
#include "lowlevel.h"
#include "dosio.h"
#include "chkfile.h"
#include "colbuf.h"
#include "vooalloc.h"
#include "debug.h"
#include "easyfile.h"
#include "oops.h"
#include "u7manage.h"
#include "routine.h"

/* linkdep1 has no entry for a function missing from the usecode */
#define NO_ROUTINE  0xffff

struct InputFile : DataFile {
	InputFile(char *name) : DataFile(name, 1) {}
};

/* a linkdep1 record: a function's first linkdep2 entry and its place in the cache block; the
 * next record's first entry ends the list */
struct LinkRecord {
	uint16_t first, offset;
};

char Linkdep1FileName[] = "linkdep1.";
char Linkdep2FileName[] = "linkdep2.";
char UsecodeFileName[] = "usecode.";
int32_t Linkdep1Block = 0, Linkdep2Block = 0;
uint16_t Linkdep1Count = 0, Linkdep2Size = 0;
int32_t UnusedRoutineGlobal = 0;
RoutineEntry LinkedRoutines[LINKED_ROUTINE_COUNT] = { 0 };

void AllocateLinkdep1(uint16_t bytes)
{
	Linkdep1Block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (Linkdep1Block == 0)
		ReportOutOfVoodooMemory();
}

void AllocateLinkdep2(uint16_t bytes)
{
	Linkdep2Block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (Linkdep2Block == 0)
		ReportOutOfVoodooMemory();
}

void ReadLinkdep1(DataFile *input, uint16_t bytes)
{
	ReadFileToVoodoo(input, Linkdep1Block, bytes);
}

void ReadLinkdep2(DataFile *input, uint16_t bytes)
{
	ReadFileToVoodoo(input, Linkdep2Block, bytes);
}

void LoadLinkdep()
{
	InputFile first(BuildPath(StaticPath, Linkdep1FileName, 0));
	uint16_t bytes = first.getLength();
	Linkdep1Count = (bytes - sizeof(LinkRecord)) / sizeof(LinkRecord);
	AllocateLinkdep1(bytes);
	ReadLinkdep1(&first, bytes);
	InputFile second(BuildPath(StaticPath, Linkdep2FileName, 0));
	Linkdep2Size = second.getLength();
	AllocateLinkdep2(Linkdep2Size);
	ReadLinkdep2(&second, Linkdep2Size);
}

void LookupLinkdep1(uint16_t id, uint16_t *first, uint16_t *count, uint16_t *offset)
{
	uint16_t end;

	*first = PeekWord(Linkdep1Block + id * sizeof(LinkRecord));
	*offset = PeekWord(Linkdep1Block + id * sizeof(LinkRecord) + 2);
	end = PeekWord(Linkdep1Block + id * sizeof(LinkRecord) + 4);
	*count = end - *first;
}

int32_t GetUsecodeOffset(uint16_t index)
{
	return PeekLong(Linkdep2Block + index * sizeof(int32_t));
}

UsecodeRoutine::UsecodeRoutine()
{
	length = 0;
	handle = -1;
}

UsecodeRoutine::~UsecodeRoutine()
{
	if (handle != -1)
		gShapeManager.releaseBlock(handle);
}

void UsecodeRoutine::append(DataFile *input, uint16_t bytes)
{
	int32_t address = gShapeManager.get(handle) + length;

	ReadFileToVoodoo(input, address, bytes);
}

int16_t UsecodeRoutine::available(uint16_t id)
{
	uint16_t first, count, offset;

	LookupLinkdep1(id, &first, &count, &offset);
	return offset != NO_ROUTINE;
}

/* Load function id and the functions it calls, from the cache when they are there. The cached
 * block holds the code, then LinkedRoutines, then this. */
uint8_t UsecodeRoutine::load(uint16_t id)
{
	uint16_t first, count, offset;

	LookupLinkdep1(id, &first, &count, &offset);
	if (offset == NO_ROUTINE)
		return 0;
	handle = gShapeManager.reclaimBlock(id);
	if (handle != 0) {
		CopyLinearToFar(LinkedRoutines, gShapeManager.get(handle) + offset, sizeof(LinkedRoutines));
		CopyLinearToFar(this, gShapeManager.get(handle) + offset + sizeof(LinkedRoutines), sizeof(*this));
		return 1;
	} else {
		handle = gShapeManager.allocateBlock(offset + sizeof(LinkedRoutines) + sizeof(*this), id, 0);
		InputFile input(BuildPath(StaticPath, UsecodeFileName, 0));
		memset(LinkedRoutines, -1, sizeof(LinkedRoutines));
		uint16_t entry = NO_ROUTINE;
		int16_t n = 0;
		uint16_t i;
		for (i = first; i < first + count; i++) {
			if (n >= LINKED_ROUTINE_COUNT)
				ReportErrorSubtype(0x6305, id);
			int32_t fileOffset = GetUsecodeOffset(i);
			input.seek(fileOffset);
			int16_t routineId = input.readWord();
			LinkedRoutines[n].id = routineId;
			if ((uint16_t)routineId == id)
				entry = length;
			uint16_t bytes = input.readWord();
			append(&input, bytes);
			LinkedRoutines[n].offset = length;
			length += bytes;
			n++;
		}
		position = entry;
		CopyFarToLinear(gShapeManager.get(handle) + offset, LinkedRoutines, sizeof(LinkedRoutines));
		CopyFarToLinear(gShapeManager.get(handle) + offset + sizeof(LinkedRoutines), this, sizeof(*this));
		return 1;
	}
}

/* where called function index starts: an index into the extern table at externs, or the
 * function's number itself when direct */
uint16_t UsecodeRoutine::resolve(uint16_t index, uint16_t externs, uint8_t direct)
{
	uint16_t id;
	uint16_t i;

	if (!direct)
		id = PeekWord(gShapeManager.get(handle) + externs + index * sizeof(int16_t));
	else
		id = index;
	for (i = 0; i < LINKED_ROUTINE_COUNT; i++) {
		if ((uint16_t)(LinkedRoutines[i].id) == id)
			return LinkedRoutines[i].offset;
	}
	ReportErrorSubtype(0x6306, index);
	return 0;
}

uint8_t UsecodeRoutine::readByte()
{
	return PeekByte(gShapeManager.get(handle) + position++);
}

int16_t UsecodeRoutine::readWord()
{
	position += 2;
	return PeekWord(gShapeManager.get(handle) + position - 2);
}

/* the linear address of string index in the text whose length word is at offset */
int32_t UsecodeRoutine::text(uint16_t offset, uint16_t index)
{
	return gShapeManager.get(handle) + offset + index + 2;
}

void InitUsecodeIndex() { LoadLinkdep(); }
