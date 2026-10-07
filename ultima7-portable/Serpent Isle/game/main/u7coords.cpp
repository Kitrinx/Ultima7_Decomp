/* Serpent Isle SI.EXE, resident segment 33 (file offsets 0x019a69 to 0x019aa2, 57 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdio.h>
#include "dosio.h"
#include "dbgfont.h"
#include "coord.h"
#include "mapview.h"

extern const char CoordFormat[] = "Coord(hex):%X,%X\nCoord(dec):%d,%d";

void ShowScreenCoords(void)
{
	int16_t x, y;

	x = MainWorldView.centerX;
	y = MainWorldView.centerY;
	sprintf(WorkString, CoordFormat, x, y, x, y);
	DrawRomText((char *) WorkString, 0, 1);
}
