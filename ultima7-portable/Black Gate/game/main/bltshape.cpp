/* Black Gate U7.EXE, resident segment 5 (file offsets 0x00cda9 to 0x00d904, 2907 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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

char *ShapesFileName = "SHAPES.VGA";
char *XformFileName = "XFORM.TBL";
int16_t DrawTranslation = 0;
View *DrawTarget = &Viewport;
extern U7ShapeManager gShapeManager;
int16_t DrawType, DrawFrameNumber, DrawCellX, DrawCellY;
int8_t DrawMirrored, DrawTranslucent;
int16_t DrawZOffset;

void SetDrawTargetScreen() { DrawTarget = &ScreenView; }

void SetDrawTargetViewport() { DrawTarget = &Viewport; }

int16_t ShapeManager_getFrameCount(ShapeManager *shapes, int16_t shape)
{
	shapes->find(shape);
	if (IsTileShape(shape)) {
		return shapes->cur->size >> 6;
	} else {
		return GetShapeFrameCount(shapes->cur->data, 1);
	}
}

void ShapeManager::init(RecordCache *pool, char *path, int16_t count)
{
	ResourceManager::init(pool, count);
	directory = path;
	LoadXformTables(&translations, BuildPath(directory, XformFileName, 0), 17, 0);
	FrameFlagTable.init();
}

void ShapeManager_reloadFrame(ShapeManager *shapes, int16_t frame)
{
	int16_t shape = shapes->cur->id;
	Cache_freeEntry(shapes->own, &shapes->cur);
	shapes->use(shape);
	if (IsObjectShape(shape)) {
		SetFlatBit(FrameFlagTable.data + ((int32_t)shape << 2), frame);
	}
}

inline void ShapeManager::ensureFrame(int16_t frame)
{
	if (PeekLong(cur->data + (frame + 1) * sizeof(int32_t)) == 0) {
		ShapeManager_reloadFrame(this, frame);
	}
}

int32_t ShapeManager::locate(int16_t shape, int16_t frame)
{
	find(shape);
	if (IsTileShape(shape)) {
		return cur->data + (frame << 6);
	} else {
		ensureFrame(frame);
		return cur->data + PeekLong(cur->data + (frame + 1) * sizeof(int32_t));
	}
}

void ShapeManager_preload(ShapeManager *shapes, int16_t percent)
{
	int16_t shape = 0;
	while (shape < 1024 && shapes->own->percentage() < percent) {
		shapes->use(shape++);
	}
}

void ShapeManager_drawCurrent(ShapeManager *shapes)
{
	int16_t x, y;
	shapes->select(DrawType);
	if (shapes->cur == 0) {
		return;
	}
	if (IsTileShape(DrawType)) {
		if (DrawCellY >= 25 || DrawCellX >= 40) {
			return;
		}
		DrawTile(shapes->cur->data + (DrawFrameNumber << 6), DrawCellX, DrawCellY);
	} else {
		shapes->ensureFrame(DrawFrameNumber);
		if (IsObjectShape(DrawType)) {
			SetFlatBit(FrameFlagTable.data + ((int32_t)DrawType << 2), DrawFrameNumber);
		}
		x = (DrawCellX << 3) - DrawZOffset + 7;
		y = (DrawCellY << 3) - DrawZOffset + 7;
		if (DrawMirrored) {
			if (DrawTranslation) {
				DrawFrameFlippedTranslated(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber,
					shapes->translations + (DrawTranslation << 8), 17);
			} else if (DrawTranslucent) {
				DrawFrameFlippedTranslucent(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber,
					shapes->translations, 17);
			} else {
				DrawFrameFlipped(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber, 17);
			}
		} else {
			if (DrawTranslation) {
				DrawFrameTranslated(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber,
					shapes->translations + (DrawTranslation << 8), 17);
			} else if (DrawTranslucent) {
				DrawFrameTranslucent(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber, shapes->translations, 17);
			} else {
				DrawFrame(DrawTarget, x, y, shapes->cur->data, DrawFrameNumber, 17);
			}
		}
	}
}

void ShapeManager_drawAtCell(ShapeManager *shapes, int16_t shape, int16_t frame, int16_t x, int16_t y, int8_t mirrored)
{
	DrawType = shape;
	DrawFrameNumber = frame;
	DrawCellX = x;
	DrawCellY = y;
	DrawMirrored = mirrored;
	DrawTranslucent = (int8_t)IsTypeShape(shape) && (int8_t)gItemTypeInfo[DrawType].translucent;
	ShapeManager_drawCurrent(shapes);
}

void ShapeManager_draw(ShapeManager *shapes, View *destination, int16_t x, int16_t y,
	int16_t shape, int16_t frame, int8_t mirrored, int16_t translation)
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
		DrawTile(shapes->cur->data + (frame << 6), x, y);
	} else {
		shapes->ensureFrame(frame);
		if (IsObjectShape(shape)) {
			SetFlatBit(FrameFlagTable.data + ((int32_t)shape << 2), frame);
		}
		saved = shapes->cur;
		// The three view handles stand in for their views.
		if ((uintptr_t) destination < FIRST_BLOCK + 3)
			destination = gShapeManager.lockView((int16_t)(uintptr_t) destination);
		shapes->cur = saved;
		if (mirrored) {
			if (IsTypeShape(shape) && !(uint8_t)gItemTypeInfo[shape].translucent) {
				if (translation) {
					DrawFrameFlippedTranslated(destination, x, y, shapes->cur->data, frame,
						shapes->translations + (translation << 8), 17);
				} else {
					DrawFrameFlipped(destination, x, y, shapes->cur->data, frame, 17);
				}
			} else {
				DrawFrameFlippedTranslucent(destination, x, y, shapes->cur->data, frame, shapes->translations, 17);
			}
		} else {
			if (IsTypeShape(shape) && !(uint8_t)gItemTypeInfo[shape].translucent) {
				if (translation) {
					DrawFrameTranslated(destination, x, y, shapes->cur->data, frame,
						shapes->translations + (translation << 8), 17);
				} else {
					DrawFrame(destination, x, y, shapes->cur->data, frame, 17);
				}
			} else {
				DrawFrameTranslucent(destination, x, y, shapes->cur->data, frame, shapes->translations, 17);
			}
		}
	}
}

void ShapeManager_drawInViewport(ShapeManager *shapes, int16_t x, int16_t y, int16_t shape, int16_t frame, int8_t mirrored,
	int16_t translation)
{
	ShapeManager_draw(shapes, &Viewport, x, y, shape, frame, mirrored, translation);
}

void ShapeManager_drawItem(ShapeManager *shapes, int16_t x, int16_t y, objref item, View *destination)
{
	int16_t translation;
	if ((uint8_t)(Item_getQualityFlags(&item) & QUALITY_INVISIBLE)) {
		translation = 1;
	} else {
		translation = 0;
	}
	ShapeManager_draw(shapes, destination, x, y, ITEM(item.off)->typeFrame & 0x3ff,
		(ITEM(item.off)->typeFrame & 0x7c00) >> 10, 0, translation);
}
