/* Serpent Isle SI.EXE, resident segment 47 (file offsets 0x020baf to 0x020cb8, 265 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 */

#include "u7port.h"
#include "coord.h"

const int16_t UnusedCoordTable[17] = { 101, 98, 95, 92, 89, 84, 81, 78, 75, 70, 55, 53, 50, 42, 39, 21, 16 };

/* how far a lies past b, the short way round */
int16_t CompareWorldCoords(int16_t *a, int16_t *b)
{
	int16_t v;

	v = *a - *b;
	if (v < 0)
		v += WORLD_SIZE;
	if (v >= WORLD_SIZE / 2)
		v -= WORLD_SIZE;
	return v;
}

/* larger of the two wrapped axis distances */
int16_t GetGreatestDelta2D(int16_t *x1, int16_t *y1, int16_t *x2, int16_t *y2)
{
	int16_t x, y;
	int16_t xdist, ydist;

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
int16_t GetGreatestDelta3D(int16_t *x1, int16_t *y1, int16_t z1, int16_t *x2, int16_t *y2, int16_t z2)
{
	int16_t xdist, x;
	int16_t ydist, y;
	int16_t zdist;

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
