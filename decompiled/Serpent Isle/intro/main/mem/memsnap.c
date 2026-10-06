/* Serpent Isle INTRO.EXE, resident segment 40 (file offsets 0x00e67d to 0x00e797, 282 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include "memsys.h"

void MemoryStats::snapshot()
{
	if (Memory.isAttached(NEAR_MEMORY))
		bytes[NEAR_MEMORY] = Memory.available(NEAR_MEMORY);
	else
		bytes[NEAR_MEMORY] = 0;
	if (Memory.isAttached(FAR_MEMORY))
		bytes[FAR_MEMORY] = Memory.available(FAR_MEMORY);
	else
		bytes[FAR_MEMORY] = 0;
	if (Memory.isAttached(EMS_MEMORY))
		bytes[EMS_MEMORY] = Memory.available(EMS_MEMORY);
	else
		bytes[EMS_MEMORY] = 0;
}

long MemoryStats::get(unsigned char type)
{
	if (type == NEAR_MEMORY)
		return bytes[NEAR_MEMORY];
	if (type == FAR_MEMORY)
		return bytes[FAR_MEMORY];
	if (type == EMS_MEMORY)
		return bytes[EMS_MEMORY];
	return 0;
}

MemoryStats::MemoryStats()
{
	int type;

	for (type = 0; type < MEMORY_TYPES; type++)
		bytes[type] = 0;
}
