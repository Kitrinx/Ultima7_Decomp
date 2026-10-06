/* Serpent Isle ENDGAME.EXE, resident segment 38 (file offsets 0x00d8e2 to 0x00d9e1, 255 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "memsys.h"

/* Formats the free memory of each type now against what it was at original. */
char *ReportMemory(MemoryStats *original)
{
	MemoryStats current;
	String text;

	current.snapshot();
	text.format("'Memory: Original  Current     Used\n"
		"'NEAR       %5ld    %5ld    %5ld\n"
		"'FAR     %8ld %8ld %8ld\n"
		"'HIGH    %8ld %8ld %8ld\n",
		original->get(NEAR_MEMORY), current.get(NEAR_MEMORY),
		original->get(NEAR_MEMORY) - current.get(NEAR_MEMORY),
		original->get(FAR_MEMORY), current.get(FAR_MEMORY),
		original->get(FAR_MEMORY) - current.get(FAR_MEMORY),
		original->get(EMS_MEMORY), current.get(EMS_MEMORY),
		original->get(EMS_MEMORY) - current.get(EMS_MEMORY));
	return text;
}
