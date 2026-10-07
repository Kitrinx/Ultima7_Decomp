/* Serpent Isle SI.EXE, resident segment 155 (file offsets 0x03f154 to 0x03f2a6, 338 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "arena.h"
#include "xmmblock.h"
#include "xmminit.h"

int16_t XMSPresent = 0;
int16_t ExtendedMemoryMethod = 0;
int16_t XMSAlreadyAllocated = 0;
int32_t OverlayMemoryKb = 0;

/* Extended memory is the part of the arena above the far heap. */
int32_t OpenExtendedMemory(void)
{
	InitLinearMemory(LINEAR_MEMORY_SIZE);
	return ExtendedArea;
}

int32_t GetXMSBlockSize(void)
{
	return ExtendedAreaSize;
}

int32_t GetExtendedMemorySize(void)
{
	return GetXMSBlockSize();
}
