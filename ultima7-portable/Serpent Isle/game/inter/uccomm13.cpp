/* Serpent Isle SI.EXE, overlay segment 314 (file offsets 0x08a280 to 0x08a67f, 1023 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "coord.h"
#include "ucvalue.h"
#include "uclist.h"
#include "item.h"
#include "type.h"

#define CLASS_FLAGS(type) (ItemTypeClassFlags[gItemTypeInfo[(type) & 0x3ff].typeClass])

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

inline void SetItemType(objref *r, int16_t type)
{
	uint16_t frame = (ITEM(r->off)->typeFrame & 0xfc00) >> 10;
	ITEM(r->off)->setTypeFrame(type);
	ITEM(r->off)->asTypeFrame().setFlipFrame(frame);
}

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x0d: change an item's type within its type class, lifting it off the map and putting it back
 * when it lies there */
void UC_SetItemShape(Value *args, Value *)
{
	int16_t obj = ARG(args - 1, 1);
	int16_t newType = ARG(args - 2, 1);
	int8_t place = 1;
	int16_t x, y;
	uint8_t z;
	objref item = obj;
	int16_t type = newType & 0x3ff;

	if (CLASS_FLAGS(ITEM(item.off)->typeFrame) == CLASS_FLAGS(type)) {
		if (!IsContained(&item)) {
			x = Item_getX(item);
			y = Item_getY(item);
			z = Item_getZ(&item);
			Item_detach(&item);
		} else
			place = 0;
		SetItemType(&item, newType);
		if (place)
			PlaceItem(&item, x, y, z);
	}
}

/* 0x19: the distance between two items */
void UC_GetDist(Value *args, Value *ret)
{
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	int16_t other = GetItemRef(GetListNode(args - 2, 1));
	objref r = obj;
	objref o = other;

	ret->appendInt(Item_greatestDeltaToItem(r, o));
}

/* 0x1a: the direction from an item to another item, or to the position the second argument gives
 * when it has three elements */
void UC_FindDirection(Value *args, Value *ret)
{
	int16_t count = LinkList_count(args - 2);
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (count != 3) {
		int16_t other = GetItemRef(GetListNode(args - 2, 1));
		objref o = other;

		ret->appendInt(Item_getDirToItem(&r, o, 0));
	} else {
		Coord x = ARG(args - 2, 1);
		Coord y = ARG(args - 2, 2);

		ret->appendInt(Item_getDirToCoords(&r, x, y, 0));
	}
}

/* 0x87: as 0x1a, in the four cardinal directions only */
void UC_DirectionFrom(Value *args, Value *ret)
{
	int16_t count = LinkList_count(args - 2);
	int16_t obj = GetItemRef(GetListNode(args - 1, 1));
	objref r = obj;

	if (count != 3) {
		int16_t other = GetItemRef(GetListNode(args - 2, 1));
		objref o = other;

		ret->appendInt(Item_getDirToItem(&r, o, 1));
	} else {
		Coord x = ARG(args - 2, 1);
		Coord y = ARG(args - 2, 2);

		ret->appendInt(Item_getDirToCoords(&r, x, y, 1));
	}
}
