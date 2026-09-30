/* Black Gate U7.EXE, resident segment 55 (file offsets 0x0213a8 to 0x0213e8, 64 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include <stdio.h>
#include "dosio.h"
#include "dbgfont.h"
#include "coord.h"
#include "mapview.h"

char CoordFormat[] = "Coord:%X,%X";

void ShowScreenCoords(void)
{
	int x, y;

	x = MainWorldView.centerX;
	y = MainWorldView.centerY;
	sprintf(WorkString, CoordFormat, x, y);
	DrawRomText((char far *) WorkString, 0, 1);
}
