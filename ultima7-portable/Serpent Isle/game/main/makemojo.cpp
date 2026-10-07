/* Serpent Isle SI.EXE, resident segment 23 (file offsets 0x014ee2 to 0x015175, 659 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "dosio.h"
#include "easyfile.h"
#include "vooalloc.h"
#include "debug.h"
#include "text.h"
#include "voolook.h"
#include "oops.h"
#include "makemojo.h"

/* Arrays of fixed-size records kept in extended memory: mem is the block, bound the count. */

void CheckMojoBounds(uint32_t bound, uint32_t index)
{
	if (index >= bound) {
		DebugPrintfAtCoords(1, 6, "%s\n", GetGameText(3, 208));
		DebugPrintfAtCoords(1, 8, GetGameText(3, 209));
		DebugPrintfAtCoords(1, 9, GetGameText(3, 210), index, index, bound, bound);
		DebugPrintfAtCoordsWait(1, 10, GetGameText(3, 211),
			LoadedWeaponCount, LoadedAmmoCount, LoadedArmorCount, LoadedMonsterCount, LoadedReadyCount);
		ReportErrorSubtype(0xe30a, index);
	}
}

uint8_t MakeMojo(int32_t count, int32_t size, int32_t *mem, int32_t *bound)
{
	*bound = count;
	if ((*mem = AllocateVoodooMemory(&VoodooXmsBlock, count * size)) == 0)
		ReportOutOfVoodooMemory();
	return *mem != 0;
}

/* Reads records first to last from the file. */
uint8_t ReadMojo(int16_t fd, int32_t pos, int32_t first, int32_t last, int32_t bound, int32_t mem, int32_t size)
{
	int32_t dest;
	int32_t len;

	CheckMojoBounds(bound, first);
	CheckMojoBounds(bound, last);
	dest = mem + first * size;
	len = (last - first + 1) * size;
	return ReadHandleToVoodoo(fd, pos, len, &dest) == len;
}

/* Writes records first to last to the file. */
void WriteMojo(int16_t fd, int32_t pos, int32_t first, int32_t last, int32_t bound, int32_t mem, int32_t size)
{
	int32_t src;
	int32_t len;

	CheckMojoBounds(bound, first);
	CheckMojoBounds(bound, last);
	src = mem + first * size;
	len = (last - first + 1) * size;
	WriteHandleFromVoodoo(fd, pos, len, src);
}
