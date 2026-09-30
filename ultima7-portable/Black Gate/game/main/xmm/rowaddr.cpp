#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

/* A view's row table holds the linear address of each row, four bytes apiece. */
void SetRowAddress(uint16_t row, int32_t address, int32_t table)
{
	LinearPut32(table + (uint32_t)row * 4, (uint32_t)address);
}

int32_t GetRowAddress(int16_t row, int32_t table)
{
	return (int32_t)LinearGet32(table + (uint32_t)(uint16_t)row * 4);
}
