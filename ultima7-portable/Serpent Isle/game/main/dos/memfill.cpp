#include "u7port.h"
#include "dosio.h"

void FillFarBytes(void *dest, uint16_t count, int16_t value)
{
	memset(dest, (uint8_t) value, count);
}
