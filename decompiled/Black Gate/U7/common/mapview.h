#ifndef MAPVIEW_H
#define MAPVIEW_H

#include "chunk.h"
#include "coord.h"
#include "objref.h"
#include "iteminfo.h"

struct ItemSnapshot;

struct far TypeFrame;
struct ShapeManager;

struct RendererState {
	ShapeManager *shapes;
	unsigned char savedState[3];
	unsigned animationTick;
	RendererState() { animationTick = 0; savedState[0] |= 4; }
	int frame(int type, int frame);
};

/* a region's 16 by 16 chunk numbers */
typedef unsigned TerrainRegion[16][16];

/* The world on screen: cached chunks, the terrain under them and the centre of the view. */
struct WorldView {
	ChunkCache cache;
	RendererState renderer;
	TerrainRegion far *terrain;
	int cellX, cellY;
	Coord centerX, centerY;
	Coord originX, originY;
	/* screen pixel of a world cell */
	int cellToScreenX(int x);
	int cellToScreenY(int y);
	int getChunkAt(Coord x, Coord y);
	TypeFrame getCellAt(Coord x, Coord y);
	void paintTerrain(int x0, int y0, int x1, int y1);
	void paintItem(int handle);
	void repaintItem(int handle);
	void paint();
	void updateCells();
	void setCenter(Coord x, Coord y);
	void centerAndPaint(Coord x, Coord y);
};

extern WorldView MainWorldView;

extern int LightTotal;

unsigned char far IsItemInCellWindow(int handle);

/* An item on the map and inside the loaded cell window. */
inline int CanVisit(objref *ref)
{
	return (unsigned char)(GetItemZAndStuff(ref).kind() == LOCATION_OFF_MAP) ? 0 : IsItemInCellWindow(ref->off);
}

extern int UnusedItemResetWord;
extern unsigned char LoadedRegions[4];
extern Coord CellWindowX, CellWindowY;

extern int CeilingZ;

extern unsigned char CheatKeyFToggle;
extern unsigned char ForceCellReload;
extern unsigned char AnimationEnabled;
void far UpdateLoadedRegions(Coord x, Coord y, Coord z, TerrainRegion far *terrain);
void far UpdateAnimationEnabled();
long far ChecksumCellBuffer();

/* CellBuffer holds the terrain of CELL_WINDOW by CELL_WINDOW cells around the view. */
#define CELL_WINDOW 80
typedef unsigned CellRow[CELL_WINDOW];

extern unsigned PaintTick;
extern CellRow far *CellBuffer;
extern ItemRecord far *PaintItemRecord;
extern ItemSnapshot *PaintItemCopy;
extern int PaintFrameCount;
extern int PaintFrame;

#endif
