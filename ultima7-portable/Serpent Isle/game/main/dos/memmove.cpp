#include "u7port.h"
#include "dosio.h"

void MoveFarMemory(void *dest, const void *src, uint16_t count)
{
	memmove(dest, src, count);
}
