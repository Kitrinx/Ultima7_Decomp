#include "u7port.h"
#include "plat.h"
#include "arena.h"

/* Layout, by offset:
 *
 *   0         unused, so no allocation is ever at 0
 *   0x1000    screen, 320x200
 *   0x11000   far heap, 1 MB
 *   0x111000  extended memory, to the end
 */
#define SCREEN_AREA INT32_C(0x1000)
#define FAR_HEAP_AREA INT32_C(0x11000)
#define FAR_HEAP_AREA_SIZE INT32_C(0x100000)
#define EXTENDED_AREA (FAR_HEAP_AREA + FAR_HEAP_AREA_SIZE)

uint8_t *LinearBase = 0;
uint32_t LinearSize = 0;
uint32_t FarHeapArea, FarHeapAreaSize;
uint32_t ExtendedArea, ExtendedAreaSize;

void InitLinearMemory(uint32_t size)
{
	/* the block is allocated once and kept across programs; the areas are set on each start */
	if (LinearBase == 0) {
		if (size <= EXTENDED_AREA)
			plat_fatal("Linear memory is too small.");
#ifdef U7_ARENA_CHECK
		LinearBase = ArenaCheckAllocate(size);
#else
		LinearBase = (uint8_t *) calloc(1, size);
#endif
		if (LinearBase == 0)
			plat_fatal("Not enough memory.");
		LinearSize = size;
	}
	FarHeapArea = FAR_HEAP_AREA;
	FarHeapAreaSize = FAR_HEAP_AREA_SIZE;
	ExtendedArea = EXTENDED_AREA;
	ExtendedAreaSize = LinearSize - EXTENDED_AREA;
}

uint8_t *ScreenPixels(void)
{
	if (LinearBase == 0)
		InitLinearMemory(LINEAR_MEMORY_SIZE);
	return LINEAR(SCREEN_AREA);
}

/* Keeps the block and its size, cleared as calloc gave it. */
extern "C" void ResetArenaGlobals(void)
{
	if (LinearBase)
		memset(LinearBase, 0, LinearSize);
	FarHeapArea = 0;
	FarHeapAreaSize = 0;
	ExtendedArea = 0;
	ExtendedAreaSize = 0;
}
