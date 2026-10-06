/* Serpent Isle SI.EXE, resident segment 18 (file offsets 0x0126b3 to 0x0135a9, 3830 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "typefram.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "coord.h"
#include "npcref.h"
#include "u7npc.h"
#include "collide.h"
#include "makemojo.h"
#include "monsters.h"
#include "item.h"
#include "type.h"
#include "mapview.h"
#include "sortitem.h"
#include "voolook.h"
#include "route.h"
#include "legalmov.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))

#define SOLID(n) ((unsigned char)gItemTypeInfo[n].solid)
#define WATER(n) ((unsigned char)gItemTypeInfo[n].water)
#define HAZARD(n) ((unsigned char)gItemTypeInfo[n].field)
#define ITEM(r) ((ItemRecord far *)MK_FP(ItemBufferSegment, (r).off))
#define IS_NPC(type) ((unsigned char)((ItemTypeClassFlags[gItemTypeInfo[type].typeClass] & CLASS_NPC) != 0))

MovementState::MovementState() : itemType(0), x(0), y(0)
{
	z = 0;
	zChange = 0;
	state = 1;
	movement = NPC_WALK;
}

void MovementState::setItem(objref object)
{
	itemType = ITEM(object)->typeFrame & 0x3ff;
	oldX = x = Item_getX(object);
	oldY = y = Item_getY(object);
	oldZ = z = Item_getZ(&object);
	zChange = 0;
	state = 1;
	if (IS_NPC(ITEM(object)->typeFrame & 0x3ff))
		movement = GetNpcBufferForIbo(&NPCRef(object.off))->typeFlags;
	else
		movement = NPC_WALK;
}

char MovementState::tryStep(char dir, char rejectObstacle)
{
	char clear = 1;
	int result;
	int type;

	zChange = 0;
	type = itemType;
	if (type == 0)
		return 0;
	if (SOLID(type)) {
		RemoveTypeFromCollision(oldX, oldY, oldZ, itemType);
	}
	result = CheckMove(x, y, z, itemType, dir, 1, movement);
	zChange += result;
	clear = (result & MOVE_CLEAR) != 0;
	state = (result & MOVE_HAZARD) ? 0 : 1;
	if (result & MOVE_DOOR) {
		state = 2;
		if (rejectObstacle)
			clear = 0;
	}
	if (SOLID(type)) {
		AddTypeToCollision(oldX, oldY, oldZ, itemType);
	}
	if (clear) {
		x += DirDeltaX[dir];
		y += DirDeltaY[dir];
		z += zChange;
	}
	if (clear)
		direction = dir;
	return clear;
}

unsigned char far CanWalkAt(Loc x, Loc y, int z, TypeFrame far &typeFrame)
{
	if (IsTypeBlockedAt(x, y, z, typeFrame.bits) != 0)
		return 0;
	if (!IsTypeSupportedAt(x, y, z, typeFrame.bits))
		return 0;
	return 1;
}

unsigned char far CanFlyAt(Loc x, Loc y, int z, TypeFrame far &typeFrame)
{
	if (IsTypeBlockedAt(x, y, z, typeFrame.bits) != 0)
		return 0;
	return 1;
}

unsigned char far CanSwimAt(Loc x, Loc y, int z, TypeFrame far &typeFrame)
{
	if (z > 0)
		return 0;
	if (IsTypeBlockedAt(x, y, z, typeFrame.bits))
		return 0;
	return 1;
}

unsigned far CheckMove(CellCoord x, CellCoord y, int z, TypeFrame far &typeFrame,
	char direction, int distance, unsigned movement)
{
	Coord nextX, nextY;
	int column, row, mapX, mapY, startX, startY, width, height;
	unsigned result = MOVE_CLEAR;
	char zChange = 0;
	unsigned terrain;

	if (movement == 0)
		movement = NPC_WALK;
	direction &= 7;
	nextX = x;
	nextY = y;
	if (distance > 1) {
		if (GetFootprintX(typeFrame) + 1 < distance ||
			GetFootprintY(typeFrame) + 1 < distance)
			return 0;
		nextX += DirDeltaX[direction] * distance;
		nextY += DirDeltaY[direction] * distance;
	} else {
		nextX += DirDeltaX[direction];
		nextY += DirDeltaY[direction];
	}
	if (movement & NPC_ETHEREAL) {
		result = MOVE_CLEAR;
		return result;
	}
	if (movement & NPC_FLY) {
		if (!CanFlyAt(nextX, nextY, z, typeFrame.bits))
			result &= ~MOVE_CLEAR;
		if (!(result & MOVE_CLEAR) && z < 15 &&
			CanFlyAt(nextX, nextY, z + 1, typeFrame.bits)) {
			++z;
			++zChange;
			result |= MOVE_CLEAR;
		}
		if (!(result & MOVE_CLEAR) && z > 0 &&
			CanFlyAt(nextX, nextY, z - 1, typeFrame.bits)) {
			--z;
			--zChange;
			result |= MOVE_CLEAR;
		}
	} else {
		if (!CanWalkAt(nextX, nextY, z, typeFrame.bits))
			result &= ~MOVE_CLEAR;
		if ((movement & NPC_SWIM) && z > 0)
			result &= ~MOVE_CLEAR;
		if (movement & NPC_WALK) {
			if (!(result & MOVE_CLEAR) && z < 15 &&
				CanWalkAt(nextX, nextY, z + 1, typeFrame.bits)) {
				++z;
				++zChange;
				result |= MOVE_CLEAR;
			}
			if (!(result & MOVE_CLEAR) && z > 0 &&
				CanWalkAt(nextX, nextY, z - 1, typeFrame.bits)) {
				--z;
				--zChange;
				result |= MOVE_CLEAR;
			}
		}
	}
	if ((result & MOVE_CLEAR) && BlockedByDoor)
		result |= MOVE_DOOR;
	if ((result & MOVE_CLEAR) && z == 0) {
		mapX = startX = GetDelta(nextX, CellWindowX);
		mapY = startY = GetDelta(nextY, CellWindowY);
		width = GetFootprintX(typeFrame) + 1;
		height = GetFootprintY(typeFrame) + 1;
		for (row = 0; row < height; ++row) {
			mapX = startX;
			for (column = 0; column < width; ++column) {
				terrain = CellBuffer[mapY][mapX] & 0x3ff;
				if (!(movement & (NPC_FLY | NPC_WALK)) && !WATER(terrain))
					result &= ~MOVE_CLEAR;
				if (!(movement & (NPC_FLY | NPC_SWIM)) && SOLID(terrain))
					result &= ~MOVE_CLEAR;
				else if (HAZARD(terrain))
					result |= MOVE_HAZARD;
				if (--mapX < 0)
					break;
			}
			if (--mapY < 0)
				break;
		}
	}
	return result | (zChange & 0xff);
}

unsigned char far CanItemMoveTo(CellCoord x, CellCoord y, int z, ItemId item)
{
	unsigned char clear = 0;
	objref object(item.off);
	TypeFrame typeFrame(ITEM(object)->typeFrame);
	unsigned char movement;
	int column, row, mapX, mapY, startX, startY, width, height;
	unsigned terrain;

	if (IS_NPC(ITEM(object)->typeFrame & 0x3ff)) {
		movement = GetNpcBufferForIbo(&NPCRef(item.off))->typeFlags;
	} else
		movement = NPC_WALK;
	if (movement & NPC_ETHEREAL)
		clear = 1;
	else if (movement & NPC_FLY) {
		clear = CanFlyAt(x, y, z, typeFrame.bits) && !BlockedByDoor;
	} else {
		if (((movement & NPC_WALK) && CanWalkAt(x, y, z, typeFrame.bits)) ||
			((movement & NPC_SWIM) && CanSwimAt(x, y, z, typeFrame.bits))) {
			if (!BlockedByDoor) {
				clear = 1;
				if (z == 0) {
					mapX = startX = GetDelta(x, CellWindowX);
					mapY = startY = GetDelta(y, CellWindowY);
					width = GetFootprintX(typeFrame) + 1;
					height = GetFootprintY(typeFrame) + 1;
					for (row = 0; row < height; ++row) {
						mapX = startX;
						for (column = 0; column < width; ++column) {
							terrain = CellBuffer[mapY][mapX] & 0x3ff;
							if (!(movement & NPC_WALK) && !WATER(terrain))
								clear = 0;
							else if (!(movement & NPC_SWIM) && SOLID(terrain))
								clear = 0;
							if (--mapX < 0)
								break;
						}
						if (--mapY < 0)
							break;
					}
				}
			}
		}
	}
	return clear;
}

unsigned char far CanTypeMoveTo(CellCoord x, CellCoord y, int z, TypeFrame far &typeFrame)
{
	unsigned char clear = 0;
	unsigned char movement;
	int column, row, mapX, mapY, startX, startY, width, height;
	unsigned terrain;

	if (IS_NPC(typeFrame.type())) {
		movement = 0;
		int type = MonsterLookup.get(typeFrame.type());
		if ((unsigned char)MonsterRecords.get(type)->walk)
			movement |= NPC_WALK;
		if ((unsigned char)MonsterRecords.get(type)->swim)
			movement |= NPC_SWIM;
		if ((unsigned char)MonsterRecords.get(type)->fly)
			movement |= NPC_FLY;
		if ((unsigned char)MonsterRecords.get(type)->ethereal)
			movement |= NPC_ETHEREAL;
	} else
		movement = NPC_WALK;
	if (movement & NPC_ETHEREAL)
		clear = 1;
	else if (movement & NPC_FLY) {
		clear = CanFlyAt(x, y, z, typeFrame.bits) && !BlockedByDoor;
	} else {
		if (((movement & NPC_WALK) && CanWalkAt(x, y, z, typeFrame.bits)) ||
			((movement & NPC_SWIM) && CanSwimAt(x, y, z, typeFrame.bits))) {
			if (!BlockedByDoor) {
				clear = 1;
				if (z == 0) {
					mapX = startX = GetDelta(x, CellWindowX);
					mapY = startY = GetDelta(y, CellWindowY);
					width = GetFootprintX(typeFrame) + 1;
					height = GetFootprintY(typeFrame) + 1;
					for (row = 0; row < height; ++row) {
						mapX = startX;
						for (column = 0; column < width; ++column) {
							terrain = CellBuffer[mapY][mapX] & 0x3ff;
							if (!(movement & NPC_WALK) && !WATER(terrain))
								clear = 0;
							else if (!(movement & NPC_SWIM) && SOLID(terrain))
								clear = 0;
							if (--mapX < 0)
								break;
						}
						if (--mapY < 0)
							break;
					}
				}
			}
		}
	}
	return clear;
}
