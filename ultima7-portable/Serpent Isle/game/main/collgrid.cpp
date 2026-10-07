/* The collision grid in linear memory: 128 cells a row, a dword of bits a cell. */

#include "u7port.h"
#include "arena.h"
#include "colbuf.h"
#include "collgrid.h"

static int32_t CellAt(int16_t row, int16_t col)
{
	return CollisionGrid + ((uint32_t) (uint16_t) row << 9) + ((uint32_t) (uint16_t) col << 2);
}

/* Block walks address cells in 16 bits, as the original did. */
static int32_t BlockCellAt(int16_t row, int16_t col)
{
	return CollisionGrid + (uint16_t) ((uint16_t) ((uint16_t) (row << 7) + col) << 2);
}

void AndCollisionCell(int16_t row, int16_t col, int32_t bits)
{
	int32_t at = CellAt(row, col);

	LinearPut32(at, LinearGet32(at) & (uint32_t) bits);
}

void OrCollisionCell(int16_t row, int16_t col, int32_t bits)
{
	int32_t at = CellAt(row, col);

	LinearPut32(at, LinearGet32(at) | (uint32_t) bits);
}

uint32_t GetCollisionCell(int16_t row, int16_t col)
{
	return LinearGet32(CellAt(row, col));
}

void SetCollisionCell(int16_t row, int16_t col, int32_t bits)
{
	LinearPut32(CellAt(row, col), (uint32_t) bits);
}

/* The OR of the cells in a block whose bottom-right corner is row, col, clipped at the grid's
 * top and left edges. Both extents are one less than the size. */
int32_t OrCollisionBlock(int16_t row, int16_t col, int16_t rows, int16_t cols)
{
	uint32_t result = 0;
	int32_t at;
	int16_t c, n;

	do {
		at = BlockCellAt(row, col);
		c = col;
		n = cols;
		do {
			result |= LinearGet32(at);
			at -= 4;
		} while (--c >= 0 && --n >= 0);
	} while (--row >= 0 && --rows >= 0);
	return (int32_t) result;
}

/* 1 when every cell of such a block shares a bit with bits. */
int8_t IsCollisionBlockSet(int16_t row, int16_t col, int16_t rows, int16_t cols, int32_t bits)
{
	int32_t at;
	int16_t c, n;

	do {
		at = BlockCellAt(row, col);
		c = col;
		n = cols;
		do {
			if ((LinearGet32(at) & (uint32_t) bits) == 0)
				return 0;
			at -= 4;
		} while (--c >= 0 && --n >= 0);
	} while (--row >= 0 && --rows >= 0);
	return 1;
}
