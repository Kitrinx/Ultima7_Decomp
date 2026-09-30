/* Black Gate U7.EXE, resident segment 61 (file offsets 0x024dab to 0x024fdf, 564 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

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
	int fd;

	fd = CreateFileOrFail(BuildPath(dir, MapCoordFileName, 0));
	DosWrite(fd, -1L, 2L, &MainWorldView.centerX);
	DosWrite(fd, -1L, 2L, &MainWorldView.centerY);
	DosWrite(fd, -1L, 3L, MainWorldView.renderer.savedState);
	DosWrite(fd, -1L, 2L, &MainWorldView.renderer.animationTick);
	DosClose(fd);
}

void MapCoordsSaver::load(char *dir)
{
	int fd;

	fd = OpenFileOrFail(BuildPath(dir, MapCoordFileName, 0));
	DosRead(fd, -1L, 2L, &MainWorldView.centerX);
	DosRead(fd, -1L, 2L, &MainWorldView.centerY);
	DosRead(fd, -1L, 3L, MainWorldView.renderer.savedState);
	DosRead(fd, -1L, 2L, &MainWorldView.renderer.animationTick);
	DosClose(fd);
}

/* The world cell under screen point x, y; the view shows 40 by 24 cells of 8 pixels. */
void ScreenToWorldCoords(int x, int y, int *tx, int *ty, char snap)
{
	int xOffset, yOffset;

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

void WorldCoordsToScreen(Coord tx, Coord ty, int *x, int *y)
{
	Coord left;
	Coord top;

	left = MainWorldView.centerX;
	*x = (CompareWorldCoords(&tx, &left) + 20) * 8 + 7;
	top = MainWorldView.centerY;
	*y = (CompareWorldCoords(&ty, &top) + 12) * 8 + 7;
}
