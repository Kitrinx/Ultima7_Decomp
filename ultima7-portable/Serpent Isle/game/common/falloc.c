/* Serpent Isle SI.EXE, resident segment 52 (file offsets 0x0218ae to 0x0218ea, 60 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
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
