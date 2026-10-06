#ifndef SORTITEM_H
#define SORTITEM_H

struct Coord;
struct ShapeExtent;
struct far TypeFrame;

struct objref;
struct CellCoord;

/* the items to draw, filled from the front of the array and from its back */
struct RenderOrder {
	objref *begin;
	objref *front;
	objref *cur;
	objref *back;
	objref *end;
	RenderOrder(objref *a, int n) { begin = a; end = a + n; }
	unsigned char addItem(objref r);
	objref first();
	objref last();
	objref previous();
	objref nextItem();
	unsigned char build(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int maxZ, int minZ);
};

extern RenderOrder ItemRenderOrder;

extern int DirDeltaX[9];
extern int DirDeltaY[9];
extern int CardinalDeltaX[5];
extern int CardinalDeltaY[5];
extern objref RenderItems[1024];
extern unsigned char ContactFound;
extern objref ContactItem;
extern objref ContactOther;
extern Coord RenderOriginX;
extern Coord RenderOriginY;
extern unsigned CellSpanMasks[8];
extern unsigned ExtentHeightMasks[8];
extern int ChunkNextItem[5][5];
extern unsigned char ChunkSortState[5][5];
extern int HeldItemFirst[5][5];
extern int HeldItemSecond[5][5];
extern int LastChunkColumn;
extern int LastChunkRow;
extern unsigned RenderBoxHeight;
extern unsigned RenderBoxWidth;
extern unsigned RenderMinZ;
extern unsigned RenderMaxZ;
void far TripContactItem(void);
void far SetItemExtent(ShapeExtent *box, objref r, unsigned char x, unsigned char y);
int far CompareItemExtent(ShapeExtent *box, objref r);
unsigned char far PlaceItem(objref *ref, CellCoord x, CellCoord y);
void far UpdateMirrorFrame(objref r);
char far Item_crossesChunkX(objref r);
unsigned char far ShapeCrossesChunkX(TypeFrame far &typeFrame, unsigned char x);
char far Item_crossesChunkY(objref r);
unsigned char far ShapeCrossesChunkY(TypeFrame far &typeFrame, unsigned char y);
unsigned char far IsItemHiddenBehind(objref a, objref b);
unsigned char far ReleaseHeldItem(int col, int row, objref r);

#endif
