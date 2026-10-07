/* The depth buffer that hides shapes behind others: rows of 128 signed depth bytes in
 * linear memory. */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "cullmask.h"

#define DEPTH_PITCH 128

static int8_t DepthAt(int32_t cell) { return (int8_t) LinearGet8(cell); }

void RaiseDepth(int32_t cell, int16_t depth)
{
	if ((int8_t) depth > DepthAt(cell))
		LinearPut8(cell, (uint8_t) depth);
}

/* A run of 0 covers 65536 cells, as the loop instruction did. */
void RaiseDepthRun(int32_t cell, int16_t depth, uint16_t run)
{
	do {
		RaiseDepth(cell++, depth);
	} while (--run != 0);
}

void RaiseDepthRect(int32_t cell, int16_t depth, int16_t cols, int16_t rows)
{
	uint16_t row = rows, col;
	int32_t at;

	do {
		at = cell;
		cell += DEPTH_PITCH;
		col = cols;
		do {
			RaiseDepth(at++, depth);
		} while (--col != 0);
	} while (--row != 0);
}

int16_t IsBelowDepth(int32_t cell, int16_t depth)
{
	return (int8_t) depth < DepthAt(cell);
}

/* Below all four corner cells of a box cols across and rows down. */
int16_t IsBelowDepthCorners(int32_t cell, int16_t depth, uint16_t cols, uint16_t rows)
{
	int8_t d = (int8_t) depth;

	if (d >= DepthAt(cell) || d >= DepthAt(cell + cols))
		return 0;
	cell += (uint16_t) (rows * DEPTH_PITCH);
	if (d >= DepthAt(cell) || d >= DepthAt(cell + cols))
		return 0;
	return 1;
}

/* Tests the occlusion box's near corners at its top, its centre, then its far corners at its
 * base. Halves OcclusionBoxHeight and takes half off OcclusionBoxLength on the way. */
uint8_t IsShapeHidden(void)
{
	int32_t rows = OcclusionRows + (uint16_t) OcclusionBoxY * 4;
	int32_t row = (int32_t) LinearGet32(rows);
	uint16_t x = OcclusionBoxX;
	uint16_t half;
	int8_t depth;

	depth = (int8_t) (OcclusionBoxHeight + OcclusionBoxZ);
	OcclusionBoxHeight = (uint16_t) OcclusionBoxHeight >> 1;
	if (depth >= DepthAt(row + x))
		return 0;
	x += OcclusionBoxWidth;
	if (depth >= DepthAt(row + x))
		return 0;
	half = (uint16_t) OcclusionBoxLength >> 1;
	OcclusionBoxLength -= half;
	rows += (uint16_t) (half << 2);
	row = (int32_t) LinearGet32(rows);
	x -= (uint16_t) OcclusionBoxWidth >> 1;
	depth = (int8_t) (depth - OcclusionBoxHeight);
	if (depth >= DepthAt(row + x))
		return 0;
	rows += (uint16_t) (OcclusionBoxLength << 2);
	row = (int32_t) LinearGet32(rows);
	x = OcclusionBoxX;
	depth = (int8_t) OcclusionBoxZ;
	if (depth >= DepthAt(row + x))
		return 0;
	x += OcclusionBoxWidth;
	if (depth >= DepthAt(row + x))
		return 0;
	return 1;
}

/* All four cells of a 2x2 block are nonzero. */
int16_t IsDepthBlockSet(int32_t cell)
{
	if (LinearGet8(cell) == 0 || LinearGet8(cell + 1) == 0)
		return 0;
	cell += DEPTH_PITCH;
	if (LinearGet8(cell) == 0 || LinearGet8(cell + 1) == 0)
		return 0;
	return 1;
}
