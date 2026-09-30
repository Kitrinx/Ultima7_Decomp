/* Black Gate U7.EXE, overlay segment 323 (file offsets 0x0967d0 to 0x0971e3, 2579 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

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
	InputFile(char far *name) : DataFile(name, 1) {}
};

/* a linkdep1 record: a function's first linkdep2 entry and its place in the cache block; the
 * next record's first entry ends the list */
struct LinkRecord {
	unsigned first, offset;
};

char Linkdep1FileName[] = "linkdep1.";
char Linkdep2FileName[] = "linkdep2.";
char UsecodeFileName[] = "usecode.";
long Linkdep1Block = 0, Linkdep2Block = 0;
unsigned Linkdep1Count = 0, Linkdep2Size = 0;
long UnusedRoutineGlobal = 0;
RoutineEntry LinkedRoutines[LINKED_ROUTINE_COUNT] = { 0 };

void far AllocateLinkdep1(unsigned bytes)
{
	Linkdep1Block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (Linkdep1Block == 0)
		ReportOutOfVoodooMemory();
}

void far AllocateLinkdep2(unsigned bytes)
{
	Linkdep2Block = AllocateVoodooMemory(&VoodooXmsBlock, bytes);
	if (Linkdep2Block == 0)
		ReportOutOfVoodooMemory();
}

void far ReadLinkdep1(DataFile *input, unsigned bytes)
{
	ReadFileToVoodoo(input, Linkdep1Block, bytes);
}

void far ReadLinkdep2(DataFile *input, unsigned bytes)
{
	ReadFileToVoodoo(input, Linkdep2Block, bytes);
}

void far LoadLinkdep()
{
	InputFile first(BuildPath(StaticPath, Linkdep1FileName, 0));
	unsigned bytes = first.getLength();
	Linkdep1Count = (bytes - sizeof(LinkRecord)) / sizeof(LinkRecord);
	AllocateLinkdep1(bytes);
	ReadLinkdep1(&first, bytes);
	InputFile second(BuildPath(StaticPath, Linkdep2FileName, 0));
	Linkdep2Size = second.getLength();
	AllocateLinkdep2(Linkdep2Size);
	ReadLinkdep2(&second, Linkdep2Size);
}

void far LookupLinkdep1(unsigned id, unsigned *first, unsigned *count, unsigned *offset)
{
	unsigned end;

	*first = PeekWord(Linkdep1Block + id * sizeof(LinkRecord));
	*offset = PeekWord(Linkdep1Block + id * sizeof(LinkRecord) + 2);
	end = PeekWord(Linkdep1Block + id * sizeof(LinkRecord) + 4);
	*count = end - *first;
}

long far GetUsecodeOffset(unsigned index)
{
	return PeekLong(Linkdep2Block + index * sizeof(long));
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

void UsecodeRoutine::append(DataFile *input, unsigned bytes)
{
	ReadFileToVoodoo(input, gShapeManager.get(handle) + length, bytes);
}

int UsecodeRoutine::available(unsigned id)
{
	unsigned first, count, offset;

	LookupLinkdep1(id, &first, &count, &offset);
	return offset != NO_ROUTINE;
}

/* Load function id and the functions it calls, from the cache when they are there. The cached
 * block holds the code, then LinkedRoutines, then this. */
unsigned char UsecodeRoutine::load(unsigned id)
{
	unsigned first, count, offset;

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
		unsigned entry = NO_ROUTINE;
		int n = 0;
		unsigned i;
		for (i = first; i < first + count; i++) {
			if (n >= LINKED_ROUTINE_COUNT)
				ReportErrorSubtype(0x6305, id);
			long fileOffset = GetUsecodeOffset(i);
			input.seek(fileOffset);
			int routineId = input.readWord();
			LinkedRoutines[n].id = routineId;
			if (routineId == id)
				entry = length;
			unsigned bytes = input.readWord();
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
unsigned UsecodeRoutine::resolve(unsigned index, unsigned externs, unsigned char direct)
{
	unsigned id;
	unsigned i;

	if (!direct)
		id = PeekWord(gShapeManager.get(handle) + externs + index * sizeof(int));
	else
		id = index;
	for (i = 0; i < LINKED_ROUTINE_COUNT; i++) {
		if (LinkedRoutines[i].id == id)
			return LinkedRoutines[i].offset;
	}
	ReportErrorSubtype(0x6306, index);
}

unsigned char UsecodeRoutine::readByte()
{
	return PeekByte(gShapeManager.get(handle) + position++);
}

int UsecodeRoutine::readWord()
{
	position += 2;
	return PeekWord(gShapeManager.get(handle) + position - 2);
}

/* the linear address of string index in the text whose length word is at offset */
long UsecodeRoutine::text(unsigned offset, unsigned index)
{
	return gShapeManager.get(handle) + offset + index + 2;
}

void far InitUsecodeIndex() { LoadLinkdep(); }
