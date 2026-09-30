/* Black Gate U7.EXE, overlay segment 212 (file offsets 0x053050 to 0x0539cd, 2429 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 * Routine name from the Serpent Isle beta; filename and folder are guesses.
 */

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
	int x0, y0;
	Point() { x0 = 0; y0 = 0; }
};
struct Rect : Point {
	int x1, y1;
	Rect() : Point() { x1 = 0; y1 = 0; }
	void move(int dx, int dy) { x0 += dx; y0 += dy; x1 += dx; y1 += dy; }
};
struct Sprite {
	int shape;
	unsigned char mode;
	int lifetime, textHeight, xOffset, yOffset, item;
	unsigned char frame;
	Coord x, y;
	int dx, dy;
	unsigned char flags;
	Sprite() { shape = -1; }
	unsigned char active() { return shape != -1; }
	unsigned char isText() { return textHeight != 0; }
	void offset(int x, int y) { xOffset = x; yOffset = y; }
};

extern unsigned char SaveLoadActive;
extern objref AvatarRef;

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
inline unsigned char IsCreature(objref r) {
	return (ItemTypeClassFlags[gItemTypeInfo[((ItemRecord far *)MK_FP(ItemBufferSegment, r.off))->typeFrame & 0x3ff]
		.typeClass] & CLASS_NPC) != 0;
}

void far SpriteManager_init(SpriteManager *pool, int capacity)
{
	if (pool->sprites == 0) {
		pool->capacity = capacity;
		pool->sprites = new Sprite[pool->capacity];
		if (pool->sprites == 0) {
			ReportOutOfNearMemory();
		}
		for (int i = 0; i < pool->capacity; i++) {
			Sprite_clear(&pool->sprites[i]);
		}
	}
}

int far SpriteManager_findFreeSprite(SpriteManager *pool)
{
	if (!SaveLoadActive) {
		for (int i = 0; i < pool->capacity; i++) {
			if (!pool->sprites[i].active()) {
				return i;
			}
		}
	}
	return -1;
}

int far SpriteManager_findSpriteForItem(SpriteManager *pool, int item, unsigned char mode)
{
	if (item != 0) {
		for (int i = 0; i < pool->capacity; i++) {
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

unsigned char far SpriteManager_clearOverlaps(SpriteManager *pool, int shape, Rect *bounds)
{
	for (int i = 0; i < pool->capacity; i++) {
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

int far SpriteManager_barkOnItem(SpriteManager *pool, int item, char *text, unsigned char mode,
	int lifetime, unsigned char force)
{
	if (item == 0 || !CanVisit(&objref(item)) || IsItemOccluded(item, 0) || Item_getZ(&objref(item)) > CeilingZ) {
		return 0;
	}
	if (IsCreature(item) && !force) {
		int type = GetMonsterNumber(item);
		if ((unsigned char)(MonsterRecords.get(type)->extraFlags & 0x20) || IsNpcUnconscious(&objref(item))) {
			return 0;
		}
	}
	int index = SpriteManager_findSpriteForItem(pool, item, mode);
	if (index != -1) {
		int shape = Sprite_initBark(&pool->sprites[index], item, text, mode, lifetime);
		Rect bounds;
		Sprite_getBounds(&pool->sprites[index], &bounds);
		if (!SpriteManager_clearOverlaps(pool, shape, &bounds)) {
			Sprite_free(&pool->sprites[index]);
		}
	}
	return index;
}

int far SpriteManager_barkAtCoords(SpriteManager *pool, Coord x, Coord y, char *text, int lifetime)
{
	int index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initBarkAt(&pool->sprites[index], x, y, text, lifetime);
	}
	return index;
}

int far SpriteManager_playEdgeSprite(SpriteManager *pool, unsigned char direction, int speed, int shape,
	unsigned char frame, int lifetime, unsigned char mode)
{
	int index = SpriteManager_findFreeSprite(pool);
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
		int x, y;
		Sprite_getScreenPos(&pool->sprites[index], &x, &y);
		pool->sprites[index].offset(bounds.x0 - x, bounds.y0 - y);
	}
	return index;
}

int far SpriteManager_playSprite(SpriteManager *pool, CellCoord x, CellCoord y, int dx, int dy, int shape,
	unsigned char frame, int lifetime, unsigned char mode)
{
	int index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initMovingEffect(&pool->sprites[index], x, y, dx, dy, shape, frame, lifetime, mode);
	}
	return index;
}

int far SpriteManager_playSpriteForItem(SpriteManager *pool, int item, int x, int y, int dx, int dy, int shape,
	unsigned char frame, int lifetime, unsigned char mode)
{
	if (item == 0 || !CanVisit(&objref(item)) || IsItemOccluded(item, 0) || Item_getZ(&objref(item)) > CeilingZ) {
		return 0;
	}
	int index = SpriteManager_findFreeSprite(pool);
	if (index != -1) {
		Sprite_initItemEffect(&pool->sprites[index], item, -x, -y, dx, dy, shape, frame, mode, lifetime);
	}
	return index;
}

void far SpriteManager_stopItemSprites(SpriteManager *pool, int item, unsigned char textOnly)
{
	for (int i = 0; i < pool->capacity; i++) {
		if (pool->sprites[i].active() &&
			(pool->sprites[i].item == item || item == -1)) {
			if (!textOnly || pool->sprites[i].isText()) {
				Sprite_free(&pool->sprites[i]);
			}
		}
	}
}

void far SpriteManager_stopSprite(SpriteManager *pool, int index)
{
	if (pool->sprites[index].active()) {
		Sprite_free(&pool->sprites[index]);
	}
}

void far ClearAllBarks()
{
	SpriteManager_stopItemSprites(&gSpriteManager, -1, 1);
}

void far PlayAvatarSprite()
{
	SpriteManager_playSpriteForItem(&gSpriteManager, AvatarRef, 10, 10, -2, -2, 440, 0, 15, 5);
}

void far BarkAfterSaving()
{
	if (RollChance(4)) {
		SpriteManager_stopItemSprites(&gSpriteManager, AvatarRef, 1);
		SpriteManager_barkOnItem(&gSpriteManager, AvatarRef, GetGameText(2, 68), 5, 15, 0);
	}
}
