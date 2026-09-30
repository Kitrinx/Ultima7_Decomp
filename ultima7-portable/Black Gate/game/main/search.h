#ifndef SEARCH_H
#define SEARCH_H

#include "objref.h"
#include "coord.h"

/* Walks the items of every chunk under a rectangle, from its far corner back. */
struct AreaScan {
	objref current;
	Coord x, y;
	uint16_t width, height;
	int16_t topZ, bottomZ;
	Coord left, top, right, bottom;
	Coord cx, cy;
	uint8_t started;

	uint8_t start(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int16_t topZ, int16_t bottomZ);
	int8_t first(Coord x0, Coord y0, Coord x1, Coord y1, int16_t topZ, int16_t bottomZ);
	uint8_t nextChunk();
	int8_t next();
};

/* A search of an area or a container for items of one type, quality and frame. */
struct AreaSearch : AreaScan {
	objref container;
	uint16_t flags;
	int16_t type;
	int8_t quality;
	int16_t frame;
	uint8_t firstStep;
	uint16_t minZ, maxZ;

	AreaSearch() { current.off = 0; }
	uint8_t found() { return current.off != 0; }
	operator objref() { return objref(current.off); }
};

/* Searches of one cell and of a rectangle; FindItem steps to the next match. */
int8_t FindItemInArea(AreaSearch *search, Loc x, Loc y, int16_t flags, int16_t type, int8_t quality, int16_t frame);
int8_t FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int16_t flags, int16_t type, int8_t quality,
	int16_t frame);
int8_t FindItemInContainer(AreaSearch *search, objref container, int16_t flags, int16_t type, int8_t quality, int16_t frame);
int16_t StepContainerSearch(AreaSearch *search);
void StepAreaSearch(AreaSearch *search);
int8_t FindItem(AreaSearch *search);
int8_t FindNearestItem(AreaSearch *search, Loc x, Loc y, int16_t radius, int16_t flags, int16_t type, int8_t quality, int16_t frame);

/* The same, limited to items between two heights. */
int8_t FindItemAtPointInZRange(AreaSearch *search, int16_t *x, int16_t *y, int16_t flags, int16_t type, int8_t quality,
	int16_t frame, uint16_t minZ, uint16_t maxZ);
int8_t FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int16_t flags, int16_t type, int8_t quality,
	int16_t frame, uint16_t minZ, uint16_t maxZ);
int8_t FindNearestItemInZRange(AreaSearch *search, Loc x, Loc y, int16_t radius, int16_t flags, int16_t type, int8_t quality,
	int16_t frame, uint16_t minZ, uint16_t maxZ);
objref FindItemInChunkLists(int16_t type, int8_t quality, int16_t frame);

#endif
