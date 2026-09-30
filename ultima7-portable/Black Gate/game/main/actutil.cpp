/* Black Gate U7.EXE, overlay segment 208 (file offsets 0x051af0 to 0x052783, 3219 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "sortitem.h"
#include "random.h"
#include "u7manage.h"
#include "actqueue.h"
#include "bltshape.h"
#include "search.h"
#include "itable.h"
#include "actitem.h"
#include "collide.h"
#include "script.h"
#include "npcref.h"
#include "mapview.h"
#include "legalmov.h"
#include "actutil.h"

#define TYPE_OF(item) ((item)->typeFrame & 0x3ff)

char FoodFrames[] = {0, 2, 3, 6, 9, 11, 12, 15, 16, 17, 19, 24, 25, 27, 29, 30, 31};
char PlateFrames[] = {4, 5};
char DeskItemFrames[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 12, 13, 15};
char KitchenItemFrames[] = {0, 2, 3, 4, 7, 8};

inline void SetStatusFlags(objref *r, uint16_t clear, uint16_t set) {
	NpcBuffer *p = GetNpcBufferForIbo(r);
	p->status &= ~clear;
	p->status |= set;
}
inline uint8_t IsInParty(objref who) { return IsInParty(&who); }

uint8_t DeleteCarriedItem(objref *container, TypeFrame &wanted)
{
	objref item;
	item = FindCarriedItem(container, TypeFrame(wanted.bits));
	if (item.valid()) {
		Item_delete(&item);
		return 1;
	}
	return 0;
}

objref CreateCarriedItem(TypeFrame &wanted, objref container)
{
	uint16_t type = wanted.bits & 0x3ff;
	objref item;
	int16_t frame;
	int16_t i;

	if (type == 717)    /* plate */
		frame = PlateFrames[GenerateRandomIntegerInRange(2)];
	else if (type == 377)   /* food item */
		frame = FoodFrames[GenerateRandomIntegerInRange(17)];
	else if (type == 675) {     /* desk item */
		frame = (wanted.bits & 0x7c00) >> 10;
		for (i = 0; i < 13; i++)
			if (DeskItemFrames[i] == frame)
				break;
		if (i == 13)
			frame = DeskItemFrames[GenerateRandomIntegerInRange(13)];
	} else if (type == 863)     /* kitchen items */
		frame = KitchenItemFrames[GenerateRandomIntegerInRange(6)];
	else if ((int8_t)((wanted.bits & 0x8000) == 0x8000))
		frame = (wanted.bits & 0x7c00) >> 10;
	else
		frame = 255;
	CreateItemInContainer(&item, type, container);
	Item_setTemporary(&item);
	Item_clearOkayToTake(&item);
	if (frame == 255)
		Item_setFrame(&item, GenerateRandomIntegerInRange(ShapeManager_getFrameCount(&gShapeManager, type)));
	else
		Item_setFrame(&item, ClampShapeFrame(TYPE_OF(ITEM(item.off)), frame));
	return item;
}

objref FindCarriedItem(objref *container, TypeFrame &wanted)
{
	AreaSearch found;
	int16_t frame;
	if (wanted.flipped())
		frame = wanted.frame();
	else
		frame = 255;
	FindItemInContainer(&found, *container, 0, wanted.type(), 255, frame);
	return objref(found.current.off);
}

DropSpot FindDropSpot(objref *npc, TypeFrame &wanted)
{
	int16_t direction, end;
	uint8_t baseZ = Item_getZ(npc);
	int16_t height;
	DropSpot result;
	for (height = 0; height < 3; height++) {
		direction = GetNpcBufferForIbo(npc)->facing() & ~1;
		end = direction + 8;
		for (; direction < end; direction += 2) {
			if (CanTypeMoveTo(Coord(Item_getX(*npc) + DirDeltaX[direction & 7]),
				Coord(Item_getY(*npc) + DirDeltaY[direction & 7]), baseZ + height, wanted.bits)) {
				result.direction = direction & 7;
				result.z = baseZ + height;
				return result;
			}
		}
	}
	for (height = 0; height < 3; height++) {
		for (direction = 1; direction < 8; direction += 2) {
			if (CanTypeMoveTo(Coord(Item_getX(*npc) + DirDeltaX[direction]),
				Coord(Item_getY(*npc) + DirDeltaY[direction]), baseZ + height, wanted.bits)) {
				result.direction = direction;
				result.z = baseZ + height;
				return result;
			}
		}
	}
	result.direction = 8;
	return result;
}

NPCRef ChooseNpcInFront(objref *npc, uint8_t includeParty)
{
	AreaSearch found, scan;
	int16_t count = 0;
	NPCRef none;
	int16_t selected;
	none.off = 0;
	if (GetNpcBufferForIbo(npc)->facing() == 2)
		FindItemInArea(&found, Coord(Item_getX(*npc) + 1), Coord(Item_getY(*npc) - 7),
			Coord(Item_getX(*npc) + 20), Coord(Item_getY(*npc) + 7), 4, -1, 255, 255);
	else if (GetNpcBufferForIbo(npc)->facing() == 4)
		FindItemInArea(&found, Coord(Item_getX(*npc) - 7), Coord(Item_getY(*npc) + 1),
			Coord(Item_getX(*npc) + 7), Coord(Item_getY(*npc) + 20), 4, -1, 255, 255);
	else
		return none;
	scan = found;
	while (scan.found()) {
		if (includeParty || !IsInParty(scan))
			count++;
		FindItem(&scan);
	}
	if (count == 0)
		return none;
	selected = GenerateRandomIntegerInRange(count);
	for (;;) {
		if (!IsInParty(found) || includeParty)
			selected--;
		if (selected < 0)
			break;
		FindItem(&found);
	}
	return NPCRef(found.current);
}

void WakeUpNpc(objref *npc)
{
	AreaSearch found;
	uint16_t collision;
	Coord x, y;
	if ((uint8_t)((int8_t)Item_getHitPoints(npc) <= 0))
		return;
	SetStatusFlags(npc, NPC_ASLEEP, 0);
	if (!CanVisit(npc))
		return;
	if ((uint8_t)(Item_getQualityFlags(npc) & QUALITY_BUSY))
		ActionQueue.remove(npc->off, 1, 0);
	FindItemInArea(&found, Item_getX(*npc), Item_getY(*npc),
		Item_getX(*npc), Item_getY(*npc), 0, 1011, 255, 255);   /* bed */
	x = (int16_t)Item_getX(*npc) & ~15;
	y = (int16_t)Item_getY(*npc) & ~15;
	if (found.found()) {
		collision = CheckMove(Item_getX(*npc), Item_getY(*npc),
			Item_getZ(npc), TypeFrame(ITEM(npc->off)->typeFrame), 5, 1, GetNpcBufferForIbo(npc)->typeFlags);
		if ((collision & MOVE_CLEAR) && !(collision & MOVE_DOOR))
			StepItem(*npc, 5, (int8_t)collision, 1);
		else {
			collision = CheckMove(Item_getX(*npc), Item_getY(*npc),
				Item_getZ(npc), TypeFrame(ITEM(npc->off)->typeFrame), 4, 1, GetNpcBufferForIbo(npc)->typeFlags);
			if ((collision & MOVE_CLEAR) && !(collision & MOVE_DOOR))
				StepItem(*npc, 4, (int8_t)collision, 1);
		}
		AddChunkToCollision(x, y);
	} else {
		FindItemInArea(&found, Item_getX(*npc), Item_getY(*npc),
			Item_getX(*npc), Item_getY(*npc), 0, 696, 255, 255);    /* bed */
		if (found.found() == 0)
			return;
		collision = CheckMove(Item_getX(*npc), Item_getY(*npc),
			Item_getZ(npc), TypeFrame(ITEM(npc->off)->typeFrame), 1, 1, GetNpcBufferForIbo(npc)->typeFlags);
		if ((collision & MOVE_CLEAR) && !(collision & MOVE_DOOR))
			StepItem(*npc, 1, (int8_t)collision, 1);
		else {
			collision = CheckMove(Item_getX(*npc), Item_getY(*npc),
				Item_getZ(npc), TypeFrame(ITEM(npc->off)->typeFrame), 2, 1, GetNpcBufferForIbo(npc)->typeFlags);
			if ((collision & MOVE_CLEAR) && !(collision & MOVE_DOOR))
				StepItem(*npc, 2, (int8_t)collision, 1);
		}
		AddChunkToCollision(x, y);
	}
	PostScheduleScript(npc, MakeScript(SCRIPT_STAND_FRAME, SCRIPT_END));
}
