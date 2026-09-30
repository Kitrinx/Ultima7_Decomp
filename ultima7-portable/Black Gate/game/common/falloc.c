/* Black Gate U7.EXE, resident segment 80 (file offsets 0x02bcef to 0x02bd2b, 60 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Original folder unknown.
 */

#include "u7port.h"
#include "memapi.h"
#include "oops.h"

void *AllocateFarOrFail(uint32_t size, int16_t flags)
{
	void *p;

	p = AllocateFarHeap(size, flags);
	if (p == 0)
		ReportOutOfFarMemory();
	return p;
}

int32_t LongIdentity(int32_t value)
{
	return value;
}
