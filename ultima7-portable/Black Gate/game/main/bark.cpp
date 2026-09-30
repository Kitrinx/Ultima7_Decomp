/* Black Gate U7.EXE, overlay segment 212 (file offsets 0x053050 to 0x0539cd, 2429 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Routine name from the Serpent Isle beta; filename and folder are guesses.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "objref.h"
#include "iteminfo.h"
#include "coord.h"
#include "voolook.h"
#include "sprite.h"
#include "oops.h"
#include "random.h"
#include "text.h"
#include "u7npc.h"
#include "partymov.h"
#include "monsters.h"
#include "mapview.h"
#include "item.h"
#include "type.h"
#include "cullmask.h"
#include "makemojo.h"

struct Point {
	int16_t x0, y0;
	Point() { x0 = 0; y0 = 0; }
};
struct Rect : Point {
	int16_t x1, y1;
	Rect() : Point() { x1 = 0; y1 = 0; }
	void move(int16_t dx, int16_t dy) { x0 += dx; y0 += dy; x1 += dx; y1 += dy; }
};
struct Sprite {
	int16_t shape;
	uint8_t mode;
	int16_t lifetime, textHeight, xOffset, yOffset, item;
	uint8_t frame;
	Coord x, y;
	int16_t dx, dy;
	uint8_t flags;
	Sprite() { shape = -1; }
	uint8_t active() { return shape != -1; }
	uint8_t isText() { return textHeight != 0; }
	void offset(int16_t x, int16_t y) { xOffset = x; yOffset = y; }
};

extern uint8_t SaveLoadActive;
extern objref AvatarRef;

inline uint8_t IsCreature(objref r) {
	return (ItemTypeClassFlags[gItemTypeInfo[((ItemRecord *)ItemAt(r.off))->typeFrame & 0x3ff]
		.typeClass] & CLASS_NPC) != 0;
}

void SpriteManager_init(SpriteManager *pool, int16_t capacity)
{
	if (pool->sprites == 0) {
		pool->capacity = capacity;
		pool->sprites = new Sprite[pool->capacity];
		if (pool->sprites == 0) {
			ReportOutOfNearMemory();
		}
		for (int16_t i = 0; i < pool->capacity; i++) {
			Sprite_clear(&pool->sprites[i]);
		}
	}
}

int16_t SpriteManager_findFreeSprite(SpriteManager *pool)
{
	if (!SaveLoadActive) {
		for (int16_t i = 0; i < pool->capacity; i++) {
			if (!pool->sprites[i].active()) {
				return i;
			}
		}
	}
	return -1;
}

int16_t SpriteManager_findSpriteForItem(SpriteManager *pool, int16_t item, uint8_t mode)
{
	if (item != 0) {
		for (int16_t i = 0; i < pool->capacity; i++) {
			if (pool->sprites[i].active() && pool->sprites[i].item == item) {
				if (pool->sprites[i].mode < mode) {
					Sprite_free(&pool->sprites[i]);
					return i;
				}
				return -1;
			}
		}
	}
	return SpriteManager_findFreeSprite(pool);
}

uint8_t SpriteManager_clearOverlaps(SpriteManager *pool, int16_t shape, Rect *bounds)
{
	for (int16_t i = 0; i < pool->capacity; i++) {
		if (pool->sprites[i].active() && pool->sprites[i].shape != shape) {
			Rect other;
			Sprite_getBounds(&pool->sprites[i], &other);
			if (DoRectsOverlap(&other, bounds)) {
				Sprite_free(&pool->sprites[i]);
			}
		}
	}
	return 1;
}

int16_t SpriteManager_barkOnItem(SpriteManager *pool, int16_t item, char *text, uint8_t mode,
	int16_t lifetime, uint8_t force)
{
	if (item == 0 || !CanVisit(&objref(item)) || IsItemOccluded(item, 0) || Item_getZ(&objref(item)) > (uint16_t)CeilingZ) {
		return 0;
	}
	if (IsCreature(item) && !force) {
		int16_t type = GetMonsterNumber(item);
		if ((uint8_t)(MonsterRecords.get(type)->extraFlags & 0x20) || IsNpcUnconscious(&objref(item))) {
			return 0;
		}
	}
	int16_t index = SpriteManager_findSpriteForItem(pool, item, mode);
	if (index != -1) {
		int16_t shape = Sprite_initBark(&pool->sprites[index], item, text, mode, lifetime);
		Rect bounds;
		Sprite_getBounds(&pool->sprites[index], &bounds);
		if (!SpriteManager_clearOverlaps(pool, shape, &bounds)) {
			Sprite_free(&pool->sprites[index]);
		}
	}
	return index;
}

int16_t SpriteManager_barkAtCoords(SpriteManager *pool, Coord x, Coord y, char *text, int16_t lifetime)
{
	int16_t index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initBarkAt(&pool->sprites[index], x, y, text, lifetime);
	}
	return index;
}

int16_t SpriteManager_playEdgeSprite(SpriteManager *pool, uint8_t direction, int16_t speed, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode)
{
	int16_t index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initMovingEffect(&pool->sprites[index], MainWorldView.centerX, MainWorldView.centerY,
			DirectionDX[direction] * speed, DirectionDY[direction] * speed,
			shape, frame, lifetime, mode);
		direction = GetNpcBufferForIbo(&AvatarRef)->status & 7;
		Rect screen;
		screen.x0 = 0;
		screen.y0 = 0;
		screen.x1 = 319;
		screen.y1 = 199;
		Rect bounds;
		Sprite_getBounds(&pool->sprites[index], &bounds);
		bounds.move(-((bounds.x1 - bounds.x0 + 1) / 2), -((bounds.y1 - bounds.y0 + 1) / 2));
		while (DoRectsOverlap(&bounds, &screen)) {
			bounds.move(DirectionDX[direction] * 8, DirectionDY[direction] * 8);
		}
		if (direction == 2 || direction == 6) {
			bounds.move(0, RollRandom(199) - 99);
		} else {
			bounds.move(RollRandom(319) - 159, 0);
		}
		int16_t x, y;
		Sprite_getScreenPos(&pool->sprites[index], &x, &y);
		pool->sprites[index].offset(bounds.x0 - x, bounds.y0 - y);
	}
	return index;
}

int16_t SpriteManager_playSprite(SpriteManager *pool, CellCoord x, CellCoord y, int16_t dx, int16_t dy, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode)
{
	int16_t index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initMovingEffect(&pool->sprites[index], x, y, dx, dy, shape, frame, lifetime, mode);
	}
	return index;
}

int16_t SpriteManager_playSpriteForItem(SpriteManager *pool, int16_t item, int16_t x, int16_t y, int16_t dx, int16_t dy, int16_t shape,
	uint8_t frame, int16_t lifetime, uint8_t mode)
{
	if (item == 0 || !CanVisit(&objref(item)) || IsItemOccluded(item, 0) || Item_getZ(&objref(item)) > (uint16_t)CeilingZ) {
		return 0;
	}
	int16_t index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initItemEffect(&pool->sprites[index], item, -x, -y, dx, dy, shape, frame, mode, lifetime);
	}
	return index;
}

void SpriteManager_stopItemSprites(SpriteManager *pool, int16_t item, uint8_t textOnly)
{
	for (int16_t i = 0; i < pool->capacity; i++) {
		if (pool->sprites[i].active() &&
			(pool->sprites[i].item == item || item == -1)) {
			if (!textOnly || pool->sprites[i].isText()) {
				Sprite_free(&pool->sprites[i]);
			}
		}
	}
}

void SpriteManager_stopSprite(SpriteManager *pool, int16_t index)
{
	if (pool->sprites[index].active()) {
		Sprite_free(&pool->sprites[index]);
	}
}

void ClearAllBarks()
{
	SpriteManager_stopItemSprites(&gSpriteManager, -1, 1);
}

void PlayAvatarSprite()
{
	SpriteManager_playSpriteForItem(&gSpriteManager, AvatarRef, 10, 10, -2, -2, 440, 0, 15, 5);
}

void BarkAfterSaving()
{
	if (RollChance(4)) {
		SpriteManager_stopItemSprites(&gSpriteManager, AvatarRef, 1);
		SpriteManager_barkOnItem(&gSpriteManager, AvatarRef, GetGameText(2, 68), 5, 15, 0);
	}
}
