/* Serpent Isle SI.EXE, resident segment 4 (file offsets 0x00b6f5 to 0x00c250, 2907 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "bltshape.h"
#include "lowlevel.h"
#include "view.h"
#include "u7manage.h"
#include "rescache.h"
#include "item.h"
#include "easyfile.h"
#include "xmmblock.h"
#include "type.h"
#include "xformtbl.h"
#include "frameflg.h"

int UnusedShapeWord = -1;
char *ShapesFileName = "SHAPES.VGA";
char *XformFileName = "XFORM.TBL";
int DrawTranslation = 0;
View *DrawTarget = &Viewport;
int DrawType, DrawFrameNumber, DrawCellX, DrawCellY;
char DrawMirrored, DrawTranslucent;
int DrawZOffset;

void far SetDrawTargetScreen() { DrawTarget = &ScreenView; }

void far SetDrawTargetViewport() { DrawTarget = &Viewport; }

int far ShapeManager_getFrameCount(ShapeManager *shapes, int shape)
{
	shapes->find(shape);
	if (IsTileShape(shape)) {
		return shapes->cur->size >> 6;
	} else {
		return GetShapeFrameCount(shapes->cur->address, 1);
	}
}

void ShapeManager::init(RecordCache *pool, char *path, int count)
{
	ResourceManager::init(pool, count);
	directory = path;
	LoadXformTables(&translations, BuildPath(directory, XformFileName, 0), 17, 0);
	FrameFlagTable.init();
}

void far ShapeManager_reloadFrame(ShapeManager *shapes, int frame)
{
	int shape = shapes->cur->id;
	Cache_freeEntry(shapes->own, &shapes->cur);
	shapes->use(shape);
	if (IsObjectShape(shape)) {
		SetFlatBit(FrameFlagTable.data + ((long)shape << 2), frame);
	}
}

inline void ShapeManager::ensureFrame(int frame)
{
	if (PeekLong(cur->address + (frame + 1) * sizeof(long)) == 0) {
		ShapeManager_reloadFrame(this, frame);
	}
}

long ShapeManager::locate(int shape, int frame)
{
	find(shape);
	if (IsTileShape(shape)) {
		return cur->address + (frame << 6);
	} else {
		ensureFrame(frame);
		return cur->address + PeekLong(cur->address + (frame + 1) * sizeof(long));
	}
}

void far ShapeManager_preload(ShapeManager *shapes, int percent)
{
	int shape = 0;
	while (shape < 1024 && shapes->own->percentage() < percent) {
		shapes->use(shape++);
	}
}

void far ShapeManager_drawCurrent(ShapeManager *shapes)
{
	int x, y;
	shapes->select(DrawType);
	if (shapes->cur == 0) {
		return;
	}
	if (IsTileShape(DrawType)) {
		if (DrawCellY >= 25 || DrawCellX >= 40) {
			return;
		}
		DrawTile(shapes->cur->address + (DrawFrameNumber << 6), DrawCellX, DrawCellY);
	} else {
		shapes->ensureFrame(DrawFrameNumber);
		if (IsObjectShape(DrawType)) {
			SetFlatBit(FrameFlagTable.data + ((long)DrawType << 2), DrawFrameNumber);
		}
		x = (DrawCellX << 3) - DrawZOffset + 7;
		y = (DrawCellY << 3) - DrawZOffset + 7;
		if (DrawMirrored) {
			if (DrawTranslation) {
				DrawFrameFlippedTranslated(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber,
					shapes->translations + (DrawTranslation << 8), 17);
			} else if (DrawTranslucent) {
				DrawFrameFlippedTranslucent(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber,
					shapes->translations, 17);
			} else {
				DrawFrameFlipped(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber, 17);
			}
		} else {
			if (DrawTranslation) {
				DrawFrameTranslated(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber,
					shapes->translations + (DrawTranslation << 8), 17);
			} else if (DrawTranslucent) {
				DrawFrameTranslucent(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber, shapes->translations, 17);
			} else {
				DrawFrame(DrawTarget, x, y, shapes->cur->address, DrawFrameNumber, 17);
			}
		}
	}
}

void far ShapeManager_drawAtCell(ShapeManager *shapes, int shape, int frame, int x, int y, char mirrored)
{
	DrawType = shape;
	DrawFrameNumber = frame;
	DrawCellX = x;
	DrawCellY = y;
	DrawMirrored = mirrored;
	DrawTranslucent = (char)IsTypeShape(shape) && (char)gItemTypeInfo[DrawType].translucent;
	ShapeManager_drawCurrent(shapes);
}

void far ShapeManager_draw(ShapeManager *shapes, View *destination, int x, int y,
	int shape, int frame, char mirrored, int translation)
{
	CacheEntry *saved;
	shapes->select(shape);
	if (shapes->cur == 0) {
		return;
	}
	if (IsTileShape(shape)) {
		x >>= 3;
		y >>= 3;
		if (y >= 25 || x >= 40) {
			return;
		}
		DrawTile(shapes->cur->address + (frame << 6), x, y);
	} else {
		shapes->ensureFrame(frame);
		if (IsObjectShape(shape)) {
			SetFlatBit(FrameFlagTable.data + ((long)shape << 2), frame);
		}
		saved = shapes->cur;
		destination = (View *) ResolveViewHandle((unsigned) destination);
		shapes->cur = saved;
		if (mirrored) {
			if (IsTypeShape(shape) && !(unsigned char)gItemTypeInfo[shape].translucent) {
				if (translation) {
					DrawFrameFlippedTranslated(destination, x, y, shapes->cur->address, frame,
						shapes->translations + (translation << 8), 17);
				} else {
					DrawFrameFlipped(destination, x, y, shapes->cur->address, frame, 17);
				}
			} else {
				DrawFrameFlippedTranslucent(destination, x, y, shapes->cur->address, frame, shapes->translations, 17);
			}
		} else {
			if (IsTypeShape(shape) && !(unsigned char)gItemTypeInfo[shape].translucent) {
				if (translation) {
					DrawFrameTranslated(destination, x, y, shapes->cur->address, frame,
						shapes->translations + (translation << 8), 17);
				} else {
					DrawFrame(destination, x, y, shapes->cur->address, frame, 17);
				}
			} else {
				DrawFrameTranslucent(destination, x, y, shapes->cur->address, frame, shapes->translations, 17);
			}
		}
	}
}

void far ShapeManager_drawInViewport(ShapeManager *shapes, int x, int y, int shape, int frame, char mirrored,
	int translation)
{
	ShapeManager_draw(shapes, &Viewport, x, y, shape, frame, mirrored, translation);
}

void far ShapeManager_drawItem(ShapeManager *shapes, int x, int y, objref item, View *destination)
{
	int translation;
	if ((unsigned char)(Item_getQualityFlags(&item) & QUALITY_INVISIBLE)) {
		translation = 1;
	} else {
		translation = 0;
	}
	ShapeManager_draw(shapes, destination, x, y, ITEM(item.off)->typeFrame & 0x3ff,
		(ITEM(item.off)->typeFrame & 0x7c00) >> 10, 0, translation);
}
