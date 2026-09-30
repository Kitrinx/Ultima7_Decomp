/* Black Gate U7.EXE, resident segment 87 (file offsets 0x02f9c8 to 0x030d8c, 5060 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "objref.h"
#include "lowlevel.h"
#include "typefram.h"
#include "view.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "coord.h"
#include "u7npc.h"
#include "u7event.h"
#include "u7manage.h"
#include "bltshape.h"
#include "random.h"
#include "cullmask.h"
#include "sounds.h"
#include "wihh.h"
#include "sortitem.h"
#include "gtimer.h"
#include "itemovr2.h"
#include "palctrl.h"
#include "vitem.h"
#include "loadreg.h"
#include "type.h"
#include "item.h"
#include "maps.h"
#include "barge.h"
#include "u7ibuf.h"
#include "mapview.h"

/* the view is 40 by 25 cells, centred on centerX, centerY */
#define VIEW_HALF_WIDTH  20
#define VIEW_HALF_HEIGHT 12

/* types below this are flat terrain */
#define FIRST_OBJECT_TYPE 150

union CellPosition {
	uint16_t packed;
	struct { uint8_t x, y; } cell;
};

struct RegionId {
	uint8_t value;
	RegionId(uint8_t n) { value = n; }
};

uint16_t PaintTick;
CellRow *CellBuffer;
ItemRecord *PaintItemRecord;
ItemSnapshot *PaintItemCopy;
int16_t PaintFrameCount, PaintFrame;
int16_t UnusedItemResetWord;
uint8_t LoadedRegions[4];
extern objref AvatarRef;

inline uint8_t HasQualityFlags(objref &ref, uint8_t mask) { return Item_getQualityFlags(&ref) & mask; }
inline uint8_t GivesLight(int16_t type) { return gItemTypeInfo[type].light; }
inline uint8_t IsAmbient(int16_t type) { return gItemTypeInfo[type].hasSfx; }
inline uint8_t IsAnimated(int16_t type) { return gItemTypeInfo[type].animated != 0; }

inline uint8_t HasNpcStatus(objref &ref, uint16_t mask)
{
	return (GetNpcBufferForIbo(&ref)->status & mask) != 0;
}
inline uint8_t HasQualityFlags(objref &&ref, uint8_t mask) { return HasQualityFlags(ref, mask); }
inline uint8_t HasNpcStatus(objref &&ref, uint16_t mask) { return HasNpcStatus(ref, mask); }

/* an animated type's frame advances with the renderer's tick */
inline int16_t RendererState::frame(int16_t type, int16_t frame)
{
	if (!IsAnimated(type) || GetTypeAnimation(type) != 0)
		return frame;
	return (frame + animationTick) % (uint16_t)(ShapeManager_getFrameCount(shapes, type));
}

int16_t LightTotal = -1;
Coord CellWindowX, CellWindowY;
int16_t CeilingZ = 15;
uint8_t CheatKeyFToggle = 1;
uint8_t ForceCellReload = 0;
uint8_t AnimationEnabled = 1;

/* Keeps the 2 by 2 regions around x, y loaded, with x, y at least 32 cells inside them: saves the
 * slots no longer wanted, then loads the missing regions into the free slots. */
void UpdateLoadedRegions(Coord x, Coord y, Coord z, TerrainRegion *terrain)
{
	int16_t removed, loaded;
	int16_t originX, originY, map;
	int16_t dx, dy;
	uint8_t wanted[4];
	int16_t slot, region;

	removed = loaded = 0;
	if (CurrentRegion == 255) {
		originX = (int16_t)x & 0xff00;
		originY = (int16_t)y & 0xff00;
		map = CurrentMap;
	} else {
		originX = RegionX[CurrentRegion];
		originY = RegionY[CurrentRegion];
		map = RegionMap[CurrentRegion];
	}
	dx = Coord((int16_t)x - originX);
	if (dx >= 1536)
		dx -= 3072;
	while (dx < 32) {
		dx += 256;
		originX -= 256;
	}
	while (dx >= 464) {
		dx -= 256;
		originX += 256;
	}
	dy = Coord((int16_t)y - originY);
	if (dy >= 1536)
		dy -= 3072;
	while (dy < 32) {
		dy += 256;
		originY -= 256;
	}
	while (dy >= 464) {
		dy -= 256;
		originY += 256;
	}
	wanted[0] = GetRegionAt(originX, originY, map);
	wanted[1] = GetRegionAt(originX + 256, originY, map);
	wanted[2] = GetRegionAt(originX, originY + 256, map);
	wanted[3] = GetRegionAt(originX + 256, originY + 256, map);
	CurrentRegion = wanted[0];
	for (slot = 0; slot < 4; slot++) {
		for (region = 0; region < 4; region++) {
			if (wanted[region] == LoadedRegions[slot])
				break;
		}
		if (region == 4 && LoadedRegions[slot] != 255) {
			RegionId oldRegion = LoadedRegions[slot];
			SaveRegion(&oldRegion.value, slot, 1);
			EmptyRegionHook(&oldRegion.value, terrain + slot);
			removed++;
			LoadedRegions[slot] = 255;
		}
	}
	for (region = 0; region < 4; region++) {
		for (slot = 0; slot < 4; slot++) {
			if (wanted[region] == LoadedRegions[slot])
				break;
		}
		if (slot == 4) {
			for (slot = 0; slot < 4; slot++) {
				if (LoadedRegions[slot] == 255)
					break;
			}
			LoadedRegions[slot] = wanted[region];
			RegionId newRegion = wanted[region];
			LoadRegion(&newRegion.value, slot);
			LoadRegionMap(&newRegion.value, terrain + slot);
			loaded++;
		}
	}
	if (loaded)
		RelinkNpcsInArea();
}

int16_t WorldView::cellToScreenX(int16_t x)
{
	return (((x - centerX + VIEW_HALF_WIDTH) & 63) * 8) + 7;
}

int16_t WorldView::cellToScreenY(int16_t y)
{
	return (((y - centerY + VIEW_HALF_HEIGHT) & 63) * 8) + 7;
}

int16_t WorldView::getChunkAt(Coord x, Coord y)
{
	int16_t column, row, slot;

	slot = FindRegionSlot(x, y);
	if (slot != 255) {
		column = ((int16_t)x >> 4) & 15;
		row = ((int16_t)y >> 4) & 15;
		return terrain[slot][row][column];
	} else {
		return 0;
	}
}

struct MemoryAddress {
	int32_t address;
	MemoryAddress(int32_t n) { address = n; }
	MemoryAddress operator+(uint16_t n) { return address + n; }
	TypeFrame word(uint16_t offset) { return PeekWord(address + offset); }
};

TypeFrame WorldView::getCellAt(Coord x, Coord y)
{
	return (MemoryAddress(cache.getChunk(getChunkAt(x, y)))
			+ ((uint16_t)(int16_t)y & 15) * 32).word(((uint16_t)(int16_t)x & 15) * 2);
}

void WorldView::paintTerrain(int16_t x0, int16_t y0, int16_t x1, int16_t y1)
{
	int16_t skip;
	int16_t halfWidth = VIEW_HALF_WIDTH, halfHeight = VIEW_HALF_HEIGHT;
	uint16_t *cell;
	uint8_t animate;

	cell = CellBuffer[cellY + y0] + (cellX + x0);
	skip = CELL_WINDOW - 1 - (x1 - x0);
	/* zero offsets, still added */
	x0 += 0;
	y0 += 0;
	x1 += 0;
	y1 += 0;
	DrawMirrored = 0;
	DrawZOffset = 0;
	DrawTranslucent = 0;
	animate = GameTime.running() && !GameInput.isGumpMode();
	for (DrawCellY = y0; DrawCellY <= y1; DrawCellY++) {
		for (DrawCellX = x0; DrawCellX <= x1; DrawCellX++) {
			DrawType = *cell & 0x3ff;
			if (DrawType < FIRST_OBJECT_TYPE) {
				DrawFrameNumber = renderer.frame(DrawType, (*cell & 0x7c00) >> 10);
				renderer.shapes->drawTile(DrawType, DrawFrameNumber, DrawCellX, DrawCellY);
				if (IsAmbient(DrawType) && animate)
					PlayItemAmbientSound(DrawType, DrawFrameNumber, DrawCellX - halfWidth, DrawCellY - halfHeight);
			}
			cell++;
		}
		cell += skip;
	}
}

void WorldView::paintItem(int16_t handle)
{
	static ItemSnapshot snapshot;

	PaintItemRecord = (ItemRecord *)ItemAt(handle);
	PaintItemCopy = &snapshot;
	PaintItemCopy->position = PaintItemRecord->position;
	PaintItemCopy->typeFrame = PaintItemRecord->typeFrame;
	DrawType = snapshot.typeFrame & 0x3ff;
	if (!IsItemOccluded(handle, gItemTypeInfo[DrawType].light)) {
		if (IsAnimated(DrawType) && AnimationEnabled) {
			PaintFrameCount = ShapeManager_getFrameCount(renderer.shapes, DrawType);
			if (PaintFrameCount > 1) {
				PaintFrame = (snapshot.typeFrame & 0x7c00) >> 10;
				/* step the frame as the type's animation kind says */
				switch (GetTypeAnimation(DrawType)) {
				case 2:
					if (GenerateRandomIntegerInRange(100) >= 25)
						break;
				case 1:
					if (++PaintFrame >= PaintFrameCount)
						PaintFrame = 0;
					break;
				case 4:
					if (GenerateRandomIntegerInRange(100) >= 25)
						break;
				case 3: {
					uint16_t flags = Item_getQualityFlags(&objref(handle));
					if (flags & 4) {
						if (--PaintFrame < 0) {
							PaintFrame = 1;
							flags ^= 4;
							Item_setQualityFlags(&objref(handle), flags);
						}
					} else {
						if (++PaintFrame >= PaintFrameCount) {
							PaintFrame = PaintFrameCount - 2;
							flags ^= 4;
							Item_setQualityFlags(&objref(handle), flags);
						}
					}
					break;
				}
				case 5:
					if (PaintFrame) {
						if (++PaintFrame >= PaintFrameCount)
							PaintFrame = 0;
					} else if (GenerateRandomIntegerInRange(100) < 5) {
						++PaintFrame;
					}
					break;
				case 6:
					PaintFrame = GenerateRandomIntegerInRange(PaintFrameCount);
					break;
				case 7:
					if (GenerateRandomIntegerInRange(100) < 5)
						PaintFrame ^= 1;
					break;
				case 9:
					if (PaintFrameCount - 1 == PaintFrame && PaintFrameCount >= 8)
						PaintFrame = PaintFrameCount - 8;
					else
						PaintFrame++;
					break;
				case 10:
					if (PaintFrameCount - 1 == PaintFrame && PaintFrameCount >= 6)
						PaintFrame = PaintFrameCount - 6;
					else
						PaintFrame++;
					break;
				case 11:
					if (PaintFrame && ++PaintFrame >= PaintFrameCount)
						PaintFrame = 1;
					break;
				case 12:
					if (!(PaintTick & 3) && ++PaintFrame >= PaintFrameCount)
						PaintFrame = 0;
					break;
				case 14:
					PaintFrame = (PaintFrame + (renderer.animationTick >> 2)) % (uint16_t)PaintFrameCount;
					break;
				case 13:
					if (PaintFrameCount - 1 > PaintFrame)
						PaintFrame++;
					break;
				case 8:
					PaintFrame = GameTime.getHour();
					if (PaintFrame > PaintFrameCount)
						PaintFrame = PaintFrameCount;
					break;
				}
				if (GetTypeAnimation(DrawType) != 14)
					Item_setFrame(&objref(handle), PaintFrame);
				snapshot.typeFrame = (snapshot.typeFrame & 0x83ff) | ((PaintFrame << 10) & 0x7c00);
			}
		}
		uint8_t region = Item_getRegion(&objref(handle));
		DrawCellX = ((snapshot.position.world.x + RegionX[region] - centerX) + VIEW_HALF_WIDTH) & 63;
		DrawCellY = ((snapshot.position.world.y + RegionY[region] - centerY) + VIEW_HALF_HEIGHT) & 63;
		DrawFrameNumber = renderer.frame(DrawType, (snapshot.typeFrame & 0x7c00) >> 10);
		DrawMirrored = (snapshot.typeFrame & 0x8000) == 0x8000;
		DrawZOffset = GetItemZAndStuff(&objref(handle)).z() * 4;
		DrawTranslucent = gItemTypeInfo[DrawType].translucent;
		if (HasQualityFlags(objref(handle), QUALITY_INVISIBLE)) {
			if (objref(handle).isNpc() && !HasNpcStatus(objref(handle), NPC_IN_PARTY))
				goto after_sprite;
			DrawTranslation = 1;
		} else {
			DrawTranslation = 0;
			if (objref(handle).isNpc()) {
				DrawTranslucent = 1;
				SetNpcTint(handle);
			}
		}
		if (GivesLight(DrawType)) {
			LightTotal += GetLightStrength(DrawType, DrawFrameNumber, objref(handle));
		}
		ShapeManager_drawCurrent(renderer.shapes);
	after_sprite:
		SetNpcTint(0);
		if (HasQualityFlags(objref(handle), QUALITY_WEAPON_READY)
			|| HasQualityFlags(objref(handle), QUALITY_CARRIES_LIGHT))
			DrawHeldItem(objref(handle), DrawCellX, DrawCellY);
	} else {
		DrawFrameNumber = renderer.frame(DrawType, (snapshot.typeFrame & 0x7c00) >> 10);
		if (DrawType == 252 || DrawType == 695)
			DrawFrameNumber = (DrawFrameNumber + (renderer.animationTick >> 2)) % 3;
	}
	if (IsAmbient(DrawType) && AnimationEnabled) {
		AnimationPhase = renderer.animationTick & 3;
		PlayItemAmbientSound(DrawType, DrawFrameNumber,
			GetDelta(Item_getX(objref(handle)), centerX),
			GetDelta(Item_getY(objref(handle)), centerY));
	}
}

void UpdateAnimationEnabled()
{
	AnimationEnabled = GameTime.running() && !GameInput.isGumpMode();
}

void WorldView::repaintItem(int16_t handle)
{
	UpdateAnimationEnabled();
	paintItem(handle);
}

void WorldView::paint()
{
	objref current;
	int16_t left, top;
	int16_t oldLight;

	UpdateAnimationEnabled();
	BargeAnimationDue = 1;
	if (AnimationEnabled) {
		renderer.animationTick++;
		PaintTick++;
	}
	oldLight = LightTotal;
	LightTotal = 0;
	left = 0;
	top = 0;
	Viewport.clip.x0 = left;
	Viewport.clip.y0 = top;
	Viewport.clip.x1 = left + SCREEN_WIDTH - 1;
	Viewport.clip.y1 = top + SCREEN_HEIGHT - 1;
	if (AvatarDontMove)
		Item_detach(&AvatarRef);
	ItemRenderOrder.build(Coord((int16_t)centerX - VIEW_HALF_WIDTH), Coord((int16_t)centerY - VIEW_HALF_HEIGHT),
		Coord((int16_t)centerX + 29), Coord((int16_t)centerY + 22), CeilingZ, 0);
	if (AvatarDontMove)
		PlaceItemOffMap(&AvatarRef);
	paintTerrain(0, 0, 39, 24);
	for (current = ItemRenderOrder.last(); current.valid(); current = ItemRenderOrder.previous())
		paintItem(current.off);
	PlayAmbientSounds();
	DrawTranslation = 0;
	LightTotal >>= 5;
	if (LightTotal != oldLight)
		SetLightLevel(LightTotal);
	Viewport.clip.x0 = 0;
	Viewport.clip.y0 = 0;
	Viewport.clip.x1 = SCREEN_WIDTH - 1;
	Viewport.clip.y1 = SCREEN_HEIGHT - 1;
}

/* Recentres the cell window on 16-cell steps, reloading its terrain when it moves. */
void WorldView::updateCells()
{
	int16_t alignedX, alignedY;
	Coord oldX, oldY;
	int16_t column, row;
	uint16_t *destination;
	int32_t address;
	int16_t line;

	if (ForceCellReload)
		RegionsChanged = 1;
	alignedX = (int16_t)centerX & 0xfff0;
	alignedY = (int16_t)centerY & 0xfff0;
	CellWindowX = alignedX - 32;
	CellWindowY = alignedY - 32;
	oldX = Coord((int16_t)originX - 32);
	oldY = Coord((int16_t)originY - 32);
	if ((int16_t)originX != alignedX || (int16_t)originY != alignedY || RegionsChanged) {
		UnloadWindowChunks(oldX, oldY, CellWindowX, CellWindowY);
		UpdateLoadedRegions(centerX, centerY, CurrentMap, terrain);
		ForceCellReload = 0;
		originX = alignedX;
		originY = alignedY;
		for (row = 0; row < CELL_WINDOW; row += 16) {
			for (column = 0; column < CELL_WINDOW; column += 16) {
				destination = CellBuffer[row] + column;
				address = cache.getChunk(getChunkAt(Coord(column + (int16_t)CellWindowX), Coord(row + (int16_t)CellWindowY)));
				for (line = 0; line < 16; line++) {
					CopyLinearToFar(destination, address, INT32_C(32));
					address += 32;
					destination += CELL_WINDOW;
				}
			}
		}
		LoadWindowChunks(oldX, oldY, CellWindowX, CellWindowY, this);
	}
	cellX = GetDelta(Coord((int16_t)centerX - VIEW_HALF_WIDTH), CellWindowX);
	cellY = GetDelta(Coord((int16_t)centerY - VIEW_HALF_HEIGHT), CellWindowY);
	RegionsChanged = 0;
}

void WorldView::setCenter(Coord x, Coord y)
{
	centerX = x;
	centerY = y;
	updateCells();
}

void WorldView::centerAndPaint(Coord x, Coord y)
{
	setCenter(x, y);
	paint();
}

int32_t ChecksumCellBuffer()
{
	uint16_t *cell;
	int32_t sum;
	int16_t i;

	cell = CellBuffer[0];
	sum = 0;
	for (i = 0; i < CELL_WINDOW * CELL_WINDOW; i++) {
		sum += *cell;
		cell++;
	}
	return sum;
}

inline int16_t IsInWindow(Coord x, Coord y)
{
	if (x < CellWindowX || x >= Coord(CellWindowX + CELL_WINDOW)
		|| y < CellWindowY || y >= Coord(CellWindowY + CELL_WINDOW))
		return 0;
	return 1;
}

uint8_t IsItemInCellWindow(int16_t handle)
{
	Coord x, y;
	objref item = handle;

	Item_getXAndY(item, &x.value, &y.value);
	return IsInWindow(x, y);
}
