#ifndef MAPS_H
#define MAPS_H

#include "coord.h"

extern Coord RegionX[256];
extern Coord RegionY[256];
extern uint8_t RegionsChanged;
extern Coord CurrentMap;
extern uint8_t MapFirstRegion;

int16_t GetRegionAt(CellCoord x, CellCoord y, CellCoord map);
int8_t AddMap(int16_t size);
int8_t SetCurrentMap(Coord map);

extern Coord RegionMap[256];
extern int16_t MapCount;
uint8_t GetMapFirstRegion(Coord map);

extern int16_t MapSizes[256];
extern int16_t MapWidth;

#endif
