/* Serpent Isle SI.EXE, resident segment 66 (file offsets 0x02c53f to 0x02c833, 756 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "coord.h"
#include "loadreg.h"
#include "mapview.h"

int16_t MapSizes[256];

/* the current map's width in regions, its number and its first region */
int16_t MapWidth = 12;
Coord CurrentMap = 0;
uint8_t MapFirstRegion = 0;
/* where each region lies: x, y and map */
Coord RegionX[256];
Coord RegionY[256];
Coord RegionMap[256];
/* how many maps there are */
int16_t MapCount = 0;
uint8_t RegionsChanged = 1;

/* the first region of a map: the maps below it are laid out before it */
uint8_t GetMapFirstRegion(Coord map)
{
	uint8_t base = 0;
	int16_t i;

	for (i = 0; (int16_t)map > i; i++)
		base += MapSizes[i] * MapSizes[i];
	return base;
}

/* the region holding x, y on a map */
int16_t GetRegionAt(CellCoord x, CellCoord y, CellCoord map)
{
	if ((int8_t)(map.value == CurrentMap.value))
		return (x >> 8) + (y >> 8) * MapWidth + MapFirstRegion;
	return (x >> 8) + (y >> 8) * MapSizes[map] + GetMapFirstRegion(map);
}

/* add a map of size by size regions */
int8_t AddMap(int16_t size)
{
	int16_t col, row, x, y;
	uint8_t region;

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
int8_t SetCurrentMap(Coord map)
{
	uint8_t region[2];
	int16_t i;

	if (map < CellCoord(MapCount)) {
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

extern "C" void ResetMapsGlobals(void)
{
	int16_t i;

	memset(MapSizes, 0, sizeof MapSizes);
	MapWidth = 12;
	CurrentMap = 0;
	MapFirstRegion = 0;
	for (i = 0; i < 256; i++) {
		RegionX[i] = 0;
		RegionY[i] = 0;
		RegionMap[i] = 0;
	}
	MapCount = 0;
	RegionsChanged = 1;
}
