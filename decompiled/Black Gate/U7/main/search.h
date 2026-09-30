#ifndef SEARCH_H
#define SEARCH_H

#include "objref.h"
#include "coord.h"

/* Walks the items of every chunk under a rectangle, from its far corner back. */
struct AreaScan {
	objref current;
	Coord x, y;
	unsigned width, height;
	int topZ, bottomZ;
	Coord left, top, right, bottom;
	Coord cx, cy;
	unsigned char started;

	unsigned char start(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int topZ, int bottomZ);
	char first(Coord x0, Coord y0, Coord x1, Coord y1, int topZ, int bottomZ);
	unsigned char nextChunk();
	char next();
};

/* A search of an area or a container for items of one type, quality and frame. */
struct AreaSearch : AreaScan {
	objref container;
	unsigned flags;
	int type;
	char quality;
	int frame;
	unsigned char firstStep;
	unsigned minZ, maxZ;

	AreaSearch() { current.off = 0; }
	unsigned char found() { return current.off != 0; }
	operator objref() { return objref(current.off); }
};

/* Searches of one cell and of a rectangle; FindItem steps to the next match. */
char far FindItemInArea(AreaSearch *search, Loc x, Loc y, int flags, int type, char quality, int frame);
char far FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int flags, int type, char quality,
	int frame);
char FindItemInContainer(AreaSearch *search, objref container, int flags, int type, char quality, int frame);
int far StepContainerSearch(AreaSearch *search);
void StepAreaSearch(AreaSearch *search);
char far FindItem(AreaSearch *search);
char far FindNearestItem(AreaSearch *search, Loc x, Loc y, int radius, int flags, int type, char quality, int frame);

/* The same, limited to items between two heights. */
char far FindItemAtPointInZRange(AreaSearch *search, int *x, int *y, int flags, int type, char quality,
	int frame, unsigned minZ, unsigned maxZ);
char far FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int flags, int type, char quality,
	int frame, unsigned minZ, unsigned maxZ);
char far FindNearestItemInZRange(AreaSearch *search, Loc x, Loc y, int radius, int flags, int type, char quality,
	int frame, unsigned minZ, unsigned maxZ);
objref far FindItemInChunkLists(int type, char quality, int frame);

#endif
