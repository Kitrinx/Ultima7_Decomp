/* Serpent Isle SI.EXE, overlay segment 315 (file offsets 0x08a6d0 to 0x08accb, 1531 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "item.h"
#include "coord.h"
#include "ucvalue.h"
#include "uclist.h"
#include "bogus.h"
#include "cast.h"
#include "mapview.h"
#include "search.h"
#include "gumpmgr.h"
#include "cheat.h"
#include "equip.h"

extern objref AvatarRef;

/* the int in element i of the usecode value at v */
#define ARG(v, i)   GetListNode(v, i)->toInt()

/* the item in element 1 of the usecode value at v */
#define ITEM_ARG(v) GetItemRef(GetListNode(v, 1))

/* Usecode engine calls: args - 1 is the first argument, ret takes the result. */

/* 0x2c: make an item of the type given */
void far UC_CreateNewObject(Value *args, Value *ret)
{
	objref r;

	CreateItem(&r, ARG(args - 1, 1));
	Item_setQuantity(r, 1, 0);
	Item_setOkayToTake(&r);
	ret->appendInt(r.off);
}

/* 0x2e: take an item off the map, to be placed again */
void far UC_SetLastCreated(Value *args, Value *ret)
{
	int obj = ITEM_ARG(args - 1);
	objref r = obj;

	RunEquipUsecode(r);
	ret->appendInt(Item_detach(&r));
}

/* 0x2f: put the item being dragged on the map at x, y, z, or with a single value hand it to
 * ZapDetachedItem; when it does not fit, clear the first temporary item in view and try again */
void far UC_PopToMap(Value *args, Value *ret)
{
	objref r;
	int n = LinkList_count(args - 1);

	if (!(unsigned char)GetItemBeingDragged(&r)) {
		ret->appendInt(0);
		return;
	}
	if (n == 1) {
		ret->appendInt(ZapDetachedItem(&r));
		return;
	}
	if (n != 3) {
		ret->appendInt(0);
		return;
	}
	Coord x = ARG(args - 1, 1);
	Coord y = ARG(args - 1, 2);
	int z = ARG(args - 1, 3);

	n = PlaceItem(&r, x, y, z);
	if (!n && !IsTemporary(&r)) {
		AreaSearch s;

		FindItemInArea(&s, CellWindowX, CellWindowY, Coord(CellWindowX + (CELL_WINDOW - 1)),
			Coord(CellWindowY + (CELL_WINDOW - 1)), 0, -1, 255, 255);
		while (s.found() && !IsTemporary(&s.current))
			FindItem(&s);
		if (!s.found() || !(unsigned char)Item_delete(&s.current) || !PlaceItem(&r, x, y, z)) {
			ReportErrorSubtype(0x6401, 1);
			ret->appendInt(0);
			return;
		}
		n = 1;
	} else if (!n)
		ZapDetachedItem(&r);
	ret->appendInt(n);
}

/* 0x43: put the item being dragged into a container or NPC, whatever the weight */
void far UC_PopToEnd(Value *args, Value *ret)
{
	int obj = ITEM_ARG(args - 1);
	objref r = obj;
	unsigned char hackMover = HackMoverEnabled;

	HackMoverEnabled = 1;
	unsigned char placed = !TryToPlaceItem(r, 0, 1);
	HackMoverEnabled = hackMover;
	ret->appendInt(placed);
}

/* 0x42: put the item being dragged into a container or NPC */
void far UC_PopToNPC(Value *args, Value *ret)
{
	int obj = ITEM_ARG(args - 1);
	objref r = obj;

	if (!r.valid()) {
		ret->appendInt(0);
		return;
	}
	ret->appendInt(!TryToPlaceItem(r, 0, 1));
}

/* 0x9a: close an item's gump */
void far UC_CloseGump(Value *args, Value *)
{
	int obj = ITEM_ARG(args - 1);
	objref r = obj;

	RemoveItemDialog(r);
}

/* 0x33: the items in a container of a type, quality and frame; UC_ALL matches any */
void far UC_GetContItems(Value *args, Value *ret)
{
	int obj = ITEM_ARG(args - 1);
	objref cont = obj;
	int type = ARG(args - 2, 1);
	int qual = ARG(args - 3, 1);
	int frame = ARG(args - 4, 1);

	if (!cont.valid()) {
		ret->appendInt(0);
		return;
	}
	AreaSearch s;
	if (type == UC_ALL)
		type = -1;
	if (qual == UC_ALL)
		qual = 255;
	if (frame == UC_ALL)
		frame = 255;
	if (FindItemInContainer(&s, cont, 0, type, qual, frame)) {
		do
			ret->appendInt(s.current.off);
		while (FindItem(&s));
	}
}

/* 0x3c: the items of a type within 255 of the avatar */
void far UC_FindNearbyAvatar(Value *args, Value *ret)
{
	int type = ARG(args - 1, 1);
	AreaSearch s;
	Coord x = Item_getX(AvatarRef);
	Coord y = Item_getY(AvatarRef);

	FindItemInArea(&s, Coord(x - 255), Coord(y - 255), Coord(x + 255), Coord(y + 255),
		0, type, 255, 255);
	while (s.found()) {
		ret->appendInt(s.current.off);
		FindItem(&s);
	}
}
