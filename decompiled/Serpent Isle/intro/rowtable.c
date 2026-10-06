/* Serpent Isle INTRO.EXE, resident segment 21 (file offsets 0x00c9a1 to 0x00ca32, 145 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "view.h"

/* True when every row starts one row length after the one before it. */
unsigned char ViewRows::matches(int lastX, unsigned height)
{
	int i;

	if (rows == 0)
		return 0;
	for (i = 0; i < height; i++)
		if (rows[i] != rows[0] + i * (lastX + 1))
			return 0;
	return 1;
}

/* Builds the table of row offsets for rows width bytes long, starting at base. */
unsigned char ViewRows::build(int width, unsigned height, int base)
{
	int i;

	if (rows)
		delete rows;
	rows = new int[height];
	if (rows) {
		for (i = 0; i < height; i++)
			rows[i] = base + i * width;
		return 1;
	}
	return 0;
}
