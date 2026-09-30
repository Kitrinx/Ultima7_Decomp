/* Extended memory is the part of the arena above the far heap. */

#include "u7port.h"
#include "arena.h"
#include "xmminit.h"

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
