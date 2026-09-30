/* Black Gate U7.EXE, resident segment 92 (file offsets 0x0322f9 to 0x0325ed, 756 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "coord.h"
#include "loadreg.h"
#include "mapview.h"

int MapSizes[256];

/* the current map's width in regions, its number and its first region */
int MapWidth = 12;
Coord CurrentMap = 0;
unsigned char MapFirstRegion = 0;
/* where each region lies: x, y and map */
Coord RegionX[256];
Coord RegionY[256];
Coord RegionMap[256];
/* how many maps there are */
int MapCount = 0;
unsigned char RegionsChanged = 1;

/* the first region of a map: the maps below it are laid out before it */
unsigned char GetMapFirstRegion(Coord map)
{
	unsigned char base = 0;
	int i;

	for (i = 0; (int)map > i; i++)
		base += MapSizes[i] * MapSizes[i];
	return base;
}

/* the region holding x, y on a map */
int GetRegionAt(CellCoord x, CellCoord y, CellCoord map)
{
	if ((char)(map.value == CurrentMap.value))
		return (x >> 8) + (y >> 8) * MapWidth + MapFirstRegion;
	return (x >> 8) + (y >> 8) * MapSizes[map] + GetMapFirstRegion(map);
}

/* add a map of size by size regions */
char AddMap(int size)
{
	int col, row, x, y;
	unsigned char region;

	if (MapCount == 0)
		ForgetSavedRegions();
	region = GetMapFirstRegion(MapCount);
	if (region + size * size < 255) {
		MapSizes[MapCount] = size;
		y = 0;
		for (row = 0; row < size; row++) {
			x = 0;
			for (col = 0; col < size; col++) {
				RegionX[region] = x;
				RegionY[region] = y;
				RegionMap[region] = MapCount;
				region++;
				x += 256;
			}
			y += 256;
		}
		MapCount++;
		return 1;
	}
	return 0;
}

/* make map the current one */
char SetCurrentMap(Coord map)
{
	unsigned char region[2];
	int i;

	if (map < MapCount) {
		for (i = 0; i < 4; i++) {
			if (LoadedRegions[i] != 255) {
				region[0] = LoadedRegions[i];
				SaveRegion(region, i, 1);
			}
		}
		CurrentMap = map;
		MapFirstRegion = GetMapFirstRegion(map);
		MapWidth = MapSizes[map];
		for (i = 0; i < 4; i++)
			LoadedRegions[i] = 255;
		CurrentRegion = 255;
		RegionsChanged = 1;
		return 1;
	}
	return 0;
}
