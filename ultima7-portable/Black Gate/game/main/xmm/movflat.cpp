#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

void MoveLinearFlat(int32_t dst, int32_t src, int32_t bytes)
{
	memmove(LINEAR(dst), LINEAR(src), (uint32_t) bytes);
}
