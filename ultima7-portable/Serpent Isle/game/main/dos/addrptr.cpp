#include "u7port.h"
#include "arena.h"
#include "dosio.h"

void *LinearToPointer(int32_t linear)
{
	if (linear == 0)
		return 0;
	return LINEAR(linear);
}
