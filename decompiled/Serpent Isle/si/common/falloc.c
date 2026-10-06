/* Serpent Isle SI.EXE, resident segment 52 (file offsets 0x0218ae to 0x0218ea, 60 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "memapi.h"
#include "oops.h"

void far *AllocateFarOrFail(unsigned long size, int flags)
{
	void far *p;

	p = AllocateFarHeap(size, flags);
	if (p == 0)
		ReportOutOfFarMemory();
	return p;
}

long LongIdentity(long value)
{
	return value;
}
