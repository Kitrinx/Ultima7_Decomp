/* Serpent Isle SI.EXE, resident segment 47 (file offsets 0x020baf to 0x020cb8, 265 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "coord.h"

unsigned char far *VgaMemory = (unsigned char far *) 0xA0000000L;
int UnusedCoordTable[17] = { 101, 98, 95, 92, 89, 84, 81, 78, 75, 70, 55, 53, 50, 42, 39, 21, 16 };

/* how far a lies past b, the short way round */
int CompareWorldCoords(int *a, int *b)
{
	int v;

	v = *a - *b;
	if (v < 0)
		v += WORLD_SIZE;
	if (v >= WORLD_SIZE / 2)
		v -= WORLD_SIZE;
	return v;
}

/* larger of the two wrapped axis distances */
int GetGreatestDelta2D(int *x1, int *y1, int *x2, int *y2)
{
	int x, y;
	int xdist, ydist;

	x = *x2;
	xdist = CompareWorldCoords(x1, &x);
	y = *y2;
	ydist = CompareWorldCoords(y1, &y);
	if (xdist < 0)
		xdist = -xdist;
	if (ydist < 0)
		ydist = -ydist;
	if (xdist > ydist)
		return xdist;
	return ydist;
}

/* largest of the three axis distances; height does not wrap */
int GetGreatestDelta3D(int *x1, int *y1, int z1, int *x2, int *y2, int z2)
{
	int xdist, x;
	int ydist, y;
	int zdist;

	x = *x2;
	xdist = CompareWorldCoords(x1, &x);
	y = *y2;
	ydist = CompareWorldCoords(y1, &y);
	zdist = z1 - z2;
	if (xdist < 0)
		xdist = -xdist;
	if (ydist < 0)
		ydist = -ydist;
	if (zdist < 0)
		zdist = -zdist;
	if (xdist > ydist) {
		if (xdist > zdist)
			return xdist;
		return zdist;
	}
	if (ydist > zdist)
		return ydist;
	return zdist;
}
