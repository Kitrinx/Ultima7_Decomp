#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "chunk.h"
#include "coord.h"
#include "objref.h"
#include "iteminfo.h"

struct ItemSnapshot;

struct TypeFrame;
struct ShapeManager;

struct RendererState {
	ShapeManager *shapes;
	uint8_t savedState[3];
	uint16_t animationTick;
	RendererState() { animationTick = 0; savedState[0] |= 4; }
	int16_t frame(int16_t type, int16_t frame);
};

/* a region's 16 by 16 chunk numbers */
typedef uint16_t TerrainRegion[16][16];

/* The world on screen: cached chunks, the terrain under them and the centre of the view. */
struct WorldView {
	ChunkCache cache;
	RendererState renderer;
	TerrainRegion *terrain;
	int16_t cellX, cellY;
	Coord centerX, centerY;
	Coord originX, originY;
	/* screen pixel of a world cell */
	int16_t cellToScreenX(int16_t x);
	int16_t cellToScreenY(int16_t y);
	int16_t getChunkAt(Coord x, Coord y);
	TypeFrame getCellAt(Coord x, Coord y);
	void paintTerrain(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
	void paintItem(int16_t handle);
	void repaintItem(int16_t handle);
	void paint();
	void updateCells();
	void setCenter(Coord x, Coord y);
	void centerAndPaint(Coord x, Coord y);
};

extern WorldView MainWorldView;

extern int16_t LightTotal;

uint8_t IsItemInCellWindow(int16_t handle);

/* An item on the map and inside the loaded cell window. */
inline int16_t CanVisit(objref *ref)
{
	return (uint8_t)(GetItemZAndStuff(ref).kind() == LOCATION_OFF_MAP) ? 0 : IsItemInCellWindow(ref->off);
}

extern int16_t UnusedItemResetWord;
extern uint8_t LoadedRegions[4];
extern Coord CellWindowX, CellWindowY;

extern int16_t CeilingZ;

extern uint8_t CheatKeyFToggle;
extern uint8_t ForceCellReload;
extern uint8_t AnimationEnabled;
void UpdateLoadedRegions(Coord x, Coord y, Coord z, TerrainRegion *terrain);
void UpdateAnimationEnabled();
int32_t ChecksumCellBuffer();

/* CellBuffer holds the terrain of CELL_WINDOW by CELL_WINDOW cells around the view. */
#define CELL_WINDOW 80
typedef uint16_t CellRow[CELL_WINDOW];

extern uint16_t PaintTick;
extern CellRow *CellBuffer;
extern ItemRecord *PaintItemRecord;
extern ItemSnapshot *PaintItemCopy;
extern int16_t PaintFrameCount;
extern int16_t PaintFrame;

#endif
