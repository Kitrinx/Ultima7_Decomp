#ifndef SORTITEM_H
#define SORTITEM_H

struct Coord;
struct ShapeExtent;
struct TypeFrame;

struct objref;
struct CellCoord;

/* the items to draw, filled from the front of the array and from its back */
struct RenderOrder {
	objref *begin;
	objref *front;
	objref *cur;
	objref *back;
	objref *end;
	RenderOrder(objref *a, int16_t n) { begin = a; end = a + n; }
	uint8_t addItem(objref r);
	objref first();
	objref last();
	objref previous();
	objref nextItem();
	uint8_t build(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int16_t maxZ, int16_t minZ);
};

extern RenderOrder ItemRenderOrder;

extern int16_t DirDeltaX[9];
extern int16_t DirDeltaY[9];
extern int16_t CardinalDeltaX[5];
extern int16_t CardinalDeltaY[5];
extern objref RenderItems[1024];
extern uint8_t ContactFound;
extern objref ContactItem;
extern objref ContactOther;
extern Coord RenderOriginX;
extern Coord RenderOriginY;
extern uint16_t CellSpanMasks[8];
extern uint16_t ExtentHeightMasks[8];
extern int16_t ChunkNextItem[5][5];
extern uint8_t ChunkSortState[5][5];
extern int16_t HeldItemFirst[5][5];
extern int16_t HeldItemSecond[5][5];
extern int16_t LastChunkColumn;
extern int16_t LastChunkRow;
extern uint16_t RenderBoxHeight;
extern uint16_t RenderBoxWidth;
extern uint16_t RenderMinZ;
extern uint16_t RenderMaxZ;
void TripContactItem(void);
void SetItemExtent(ShapeExtent *box, objref r, uint8_t x, uint8_t y);
int16_t CompareItemExtent(ShapeExtent *box, objref r);
uint8_t PlaceItem(objref *ref, CellCoord x, CellCoord y);
void UpdateMirrorFrame(objref r);
int8_t Item_crossesChunkX(objref r);
uint8_t ShapeCrossesChunkX(TypeFrame &typeFrame, uint8_t x);
inline uint8_t ShapeCrossesChunkX(TypeFrame &&typeFrame, uint8_t x) { return ShapeCrossesChunkX(typeFrame, x); }
int8_t Item_crossesChunkY(objref r);
uint8_t ShapeCrossesChunkY(TypeFrame &typeFrame, uint8_t y);
inline uint8_t ShapeCrossesChunkY(TypeFrame &&typeFrame, uint8_t y) { return ShapeCrossesChunkY(typeFrame, y); }
uint8_t IsItemHiddenBehind(objref a, objref b);
uint8_t ReleaseHeldItem(int16_t col, int16_t row, objref r);

#endif
