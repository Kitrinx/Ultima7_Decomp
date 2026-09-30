/* Black Gate U7.EXE, resident segment 80 (file offsets 0x02bcef to 0x02bd2b, 60 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Original folder unknown.
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
