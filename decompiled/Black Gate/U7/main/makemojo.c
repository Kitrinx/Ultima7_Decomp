/* Black Gate U7.EXE, resident segment 32 (file offsets 0x01a039 to 0x01a298, 607 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include <dos.h>
#include "dosio.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "debug.h"
#include "voolook.h"
#include "oops.h"
#include "makemojo.h"

/* Arrays of fixed-size records kept in extended memory: mem is the block, bound the count. */

void far CheckMojoBounds(unsigned long bound, unsigned long index)
{
	int a1, a2, a3, a4;

	if (index >= bound) {
		/* whatever was last pushed below this frame */
		a1 = ((int *) _SP)[-1];
		a2 = ((int *) _SP)[-2];
		a3 = ((int *) _SP)[-3];
		a4 = ((int *) _SP)[-4];
		DebugPrintfAtCoords(1, 6, "a1 = %x, a2 =%x, a3 = %x, a4 = %x  \n", a1, a2, a3, a4);
		DebugPrintfAtCoords(1, 8, "Mojo bounds check failure:");
		DebugPrintfAtCoords(1, 9, "Index %ld (%lx), bound %ld (%lx)", index, index, bound, bound);
		DebugPrintfAtCoordsWait(1, 10, "W:%d AM:%d A:%d M:%d R:%d",
			LoadedWeaponCount, LoadedAmmoCount, LoadedArmorCount, LoadedMonsterCount, LoadedReadyCount);
		ReportErrorSubtype(0xe30a, index);
	}
}

unsigned char far MakeMojo(long count, long size, long *mem, long *bound)
{
	*bound = count;
	if ((*mem = AllocateVoodooMemory(&VoodooXmsBlock, count * size)) == 0)
		ReportOutOfVoodooMemory();
	return *mem != 0;
}

/* Reads records first to last from the file. */
unsigned char far ReadMojo(int fd, long pos, long first, long last, long bound, long mem, long size)
{
	long dest;
	long len;

	CheckMojoBounds(bound, first);
	CheckMojoBounds(bound, last);
	dest = mem + first * size;
	len = (last - first + 1) * size;
	return ReadHandleToVoodoo(fd, pos, len, &dest) == len;
}

/* Writes records first to last to the file. */
void far WriteMojo(int fd, long pos, long first, long last, long bound, long mem, long size)
{
	long src;
	long len;

	CheckMojoBounds(bound, first);
	CheckMojoBounds(bound, last);
	src = mem + first * size;
	len = (last - first + 1) * size;
	WriteHandleFromVoodoo(fd, pos, len, src);
}
