#ifndef MAPS_H
#define MAPS_H

#include "coord.h"

extern Coord RegionX[256];
extern Coord RegionY[256];
extern unsigned char RegionsChanged;
extern Coord CurrentMap;
extern unsigned char MapFirstRegion;

int GetRegionAt(CellCoord x, CellCoord y, CellCoord map);
char AddMap(int size);
char SetCurrentMap(Coord map);

extern Coord RegionMap[256];
extern int MapCount;
unsigned char GetMapFirstRegion(Coord map);

extern int MapSizes[256];
extern int MapWidth;

#endif
