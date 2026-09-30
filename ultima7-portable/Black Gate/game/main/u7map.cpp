/* Black Gate U7.EXE, resident segment 61 (file offsets 0x024dab to 0x024fdf, 564 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "dosio.h"
#include "easyfile.h"
#include "datanode.h"
#include "coord.h"
#include "mapview.h"

/* The MAPCOORD.DAT file in a saved game: where the view was centred and its animation state. */
struct MapCoordsSaver : DataNode {
	char *name();
	void load(char *dir);
	void save(char *dir);
};

MapCoordsSaver MapCoordsFile;
char *MapCoordFileName = "MAPCOORD.DAT";

char *MapCoordsSaver::name()
{
	return MapCoordFileName;
}

void MapCoordsSaver::save(char *dir)
{
	int16_t fd;

	fd = CreateFileOrFail(BuildPath(dir, MapCoordFileName, 0));
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MainWorldView.centerX);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MainWorldView.centerY);
	DosWrite(fd, -INT32_C(1), INT32_C(3), MainWorldView.renderer.savedState);
	DosWrite(fd, -INT32_C(1), INT32_C(2), &MainWorldView.renderer.animationTick);
	DosClose(fd);
}

void MapCoordsSaver::load(char *dir)
{
	int16_t fd;

	fd = OpenFileOrFail(BuildPath(dir, MapCoordFileName, 0));
	DosRead(fd, -INT32_C(1), INT32_C(2), &MainWorldView.centerX);
	DosRead(fd, -INT32_C(1), INT32_C(2), &MainWorldView.centerY);
	DosRead(fd, -INT32_C(1), INT32_C(3), MainWorldView.renderer.savedState);
	DosRead(fd, -INT32_C(1), INT32_C(2), &MainWorldView.renderer.animationTick);
	DosClose(fd);
}

/* The world cell under screen point x, y; the view shows 40 by 24 cells of 8 pixels. */
void ScreenToWorldCoords(int16_t x, int16_t y, int16_t *tx, int16_t *ty, int8_t snap)
{
	int16_t xOffset, yOffset;

	if (snap != 0) {
		xOffset = x % 8;
		yOffset = y % 8;
		if (xOffset != 0) {
			if (xOffset < 5)
				x -= 4;
		} else
			x--;
		if (yOffset != 0) {
			if (yOffset < 5)
				y -= 4;
		} else
			y--;
	}
	*tx = (x >> 3) + MainWorldView.centerX - 20;
	*tx = (*tx + WORLD_SIZE) % WORLD_SIZE;
	*ty = (y >> 3) + MainWorldView.centerY - 12;
	*ty = (*ty + WORLD_SIZE) % WORLD_SIZE;
}

void WorldCoordsToScreen(Coord tx, Coord ty, int16_t *x, int16_t *y)
{
	Coord left;
	Coord top;

	left = MainWorldView.centerX;
	*x = (CompareWorldCoords(&tx, &left) + 20) * 8 + 7;
	top = MainWorldView.centerY;
	*y = (CompareWorldCoords(&ty, &top) + 12) * 8 + 7;
}
