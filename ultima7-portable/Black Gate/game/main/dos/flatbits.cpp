/* Bits of a dword in linear memory; bit numbers wrap at 32. */

#include "u7port.h"
#include "arena.h"
#include "xmmblock.h"

void ClearFlatBit(int32_t linear, int16_t bit)
{
	LinearPut32(linear, LinearGet32(linear) & ~(UINT32_C(1) << ((uint16_t) bit & 31)));
}

void SetFlatBit(int32_t linear, int16_t bit)
{
	LinearPut32(linear, LinearGet32(linear) | UINT32_C(1) << ((uint16_t) bit & 31));
}

/* -1 when set, 0 when clear. */
int16_t TestFlatBit(int32_t linear, int16_t bit)
{
	return (LinearGet32(linear) >> ((uint16_t) bit & 31) & 1) ? -1 : 0;
}
