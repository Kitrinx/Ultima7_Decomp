/* Black Gate U7.EXE, resident segment 72 (file offsets 0x028454 to 0x02acc2, 10350 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "u7ibuf.h"
#include "usehook.h"
#include "type.h"
#include "coord.h"
#include "mapview.h"
#include "sortitem.h"
#include "search.h"
#include "collide.h"
#include "barge.h"

#define ITEM_TYPE(record) ((record)->typeFrame & 0x3ff)
#define FRAME(record) (((record)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(record) (gItemTypeInfo[ITEM_TYPE(record)].typeClass)
#define IS_CLASS(record, n) ((unsigned char)(TYPE_CLASS(record) == (n)))
#define IS_NPC(type) ((unsigned char)((ItemTypeClassFlags[gItemTypeInfo[type].typeClass] & CLASS_NPC) != 0))
#define BARGE(extra) ((BargeInfo far *)MK_FP(ItemBufferSegment, (extra)))

#define BARGE_TYPE 961
#define SAILS 251
#define SEAT 292
#define CART 774
#define DRAFT_HORSE 796
#define OTHER_CART 757

/* FindItemInArea flags */
#define FIND_EGGS       0x10
#define FIND_NO_NPCS    0x20

/* A barge's extra record: its size in cells and the direction it faces. */
struct BargeInfo {
	unsigned char width, height, unusedField1, facing;
};

unsigned char BargeAnimationDue = 1;

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far PlaceItem(objref *, Loc, Loc, int);
extern int far GetItemBeingDragged(objref *ref);
extern unsigned char far Item_detach(objref *ref);
extern void far Item_setFrame(objref *, int);
extern void far Item_setZ(objref *, int);
extern unsigned char far Item_move(objref *, CellCoord, CellCoord);
extern unsigned char far Item_move(objref *, Loc, Loc, int);

TypeFrame far RotateShapeQuarter(TypeFrame);
TypeFrame far RotateShapeHalf(TypeFrame);
TypeFrame far RotateShapeBack(TypeFrame);

inline unsigned char HasTypeClass(objref &object) { return !IS_CLASS(object.ptr(), TYPE_CLASS_NONE); }

inline char FindArea(AreaSearch *items, Loc x1, Loc y1, Loc x2, Loc y2, int flags, int type, char quality, int frame)
{
	return FindItemInArea(items, x1, y1, x2, y2, flags, type, quality, frame);
}

inline char FindArea(AreaSearch *items, Loc x1, Loc y1, Loc x2, Loc y2, int flags, int type, char quality, int frame,
	unsigned minZ, unsigned maxZ)
{
	return FindItemInArea(items, x1, y1, x2, y2, flags, type, quality, frame, minZ, maxZ);
}

inline unsigned char IsOnWater(objref &item)
{
	return (unsigned char)gItemTypeInfo[CellBuffer[Item_getY(item) - CellWindowY][Item_getX(item) - CellWindowX] &
		0x3ff].water ? 1 : 0;
}

unsigned char far Barge_getWidth(objref &barge)
{
	unsigned char width = 0;
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		width = BARGE(extra)->width;
	}
	return width;
}

void far Barge_setWidth(objref &barge, char width)
{
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		BARGE(extra)->width = width;
	}
}

unsigned char far Barge_getHeight(objref &barge)
{
	unsigned char height = 0;
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		height = BARGE(extra)->height;
	}
	return height;
}

void far Barge_setHeight(objref &barge, char height)
{
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		BARGE(extra)->height = height;
	}
}

unsigned char far Barge_getDir(objref &barge)
{
	unsigned char facing = 0;
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		facing = BARGE(extra)->facing;
	}
	return facing;
}

void far Barge_setDir(objref &barge, char facing)
{
	int extra;

	if (IS_CLASS(barge.ptr(), TYPE_CLASS_BARGE)) {
		extra = barge.ptr()->data.extra;
		BARGE(extra)->facing = facing;
	}
}

/* Can the barge move dist cells towards dir? */
char far Barge_canMove(objref &barge, unsigned char dir, int dist)
{
	Coord x0, y0, x1, y1;

	x1 = Item_getX(barge);
	y1 = Item_getY(barge);
	x0 = Coord(x1.value - Barge_getWidth(barge) + 1);
	y0 = Coord(y1.value - Barge_getHeight(barge) + 1);
	switch (dir) {
	case 0:
		y1 = y0 - 1;
		y0 -= dist;
		break;
	case 2:
		x0 = x1 + 1;
		x1 += dist;
		break;
	case 4:
		y0 = y1 + 1;
		y1 += dist;
		break;
	case 6:
		x1 = x0 - 1;
		x0 -= dist;
		break;
	case 1:
		if (!Barge_isStripFree(barge, x0 + dist, y0 - dist,
				x1 + dist, y0 - 1))
			return 0;
		x0 = x1 + 1;
		x1 += dist;
		y1 -= dist;
		break;
	case 3:
		if (!Barge_isStripFree(barge, x0 + dist, y1 + 1,
				x1 + dist, y1 + dist))
			return 0;
		x0 = x1 + 1;
		y0 += dist;
		x1 += dist;
		break;
	case 7:
		if (!Barge_isStripFree(barge, x0 - dist, y0 - dist,
				x1 - dist, y0 - 1))
			return 0;
		x1 = x0 - 1;
		x0 -= dist;
		y1 -= dist;
		break;
	case 5:
		if (!Barge_isStripFree(barge, x0 - dist, y1 + 1,
				x1 - dist, y1 + dist))
			return 0;
		x1 = x0 - 1;
		x0 -= dist;
		y0 += dist;
		break;
	}
	return Barge_isStripFree(barge, x0, y0, x1, y1);
}

/* Moves the barge and everything standing on it dist cells towards dir. */
char far Barge_move(objref &barge, unsigned char dir, int dist)
{
	objref cur;
	int i, count;
	unsigned z;
	int dx, dy;
	Coord x1, y1, x2, y2;
	unsigned type, frame;
	AreaSearch items;

	x2 = Item_getX(barge);
	y2 = Item_getY(barge);
	x1 = Coord(x2.value - Barge_getWidth(barge) + 1);
	y1 = Coord(y2.value - Barge_getHeight(barge) + 1);
	z = Item_getZ(&barge);
	if (x1 < CellWindowX || x2 >= CellWindowX + CELL_WINDOW
		|| y1 < CellWindowY || y2 >= CellWindowY + CELL_WINDOW)
		return 0;
	dx = DirDeltaX[dir] * dist;
	dy = DirDeltaY[dir] * dist;
	if (x1 + dx < CellWindowX || x2 + dx >= CellWindowX + CELL_WINDOW
		|| y1 + dy < CellWindowY || y2 + dy >= CellWindowY + CELL_WINDOW)
		return 0;

	/* Lift everything on the deck off the map. */
	count = 0;
	FindArea(&items, x1, y1, x2, y2, FIND_NO_NPCS, -1, 255, 255);
	while (items.found()) {
		cur = items.current;
		type = ITEM_TYPE(cur.ptr());
		if (HasTypeClass(cur)
			&& ((unsigned char)gItemTypeInfo[type].solid || (unsigned char)gItemTypeInfo[type].bargePart)
			&& Item_getZ(&cur) >= z) {
			count++;
			FindItem(&items);
			Item_detach(&cur);
		} else
			FindItem(&items);
	}

	/* Set it all down again at the new position; carts and draft horses also change frame. */
	for (i = 0; i < count; i++) {
		GetItemBeingDragged(&cur);
		if (BargeAnimationDue) {
			type = ITEM_TYPE(cur.ptr());
			if (type == CART) {
				frame = FRAME(cur.ptr());
				if (dir == 4 || dir == 2)
					frame++;
				else
					frame--;
				Item_setFrame(&cur, frame & 3);
			} else if (type == DRAFT_HORSE || type == OTHER_CART) {
				frame = FRAME(cur.ptr());
				Item_setFrame(&cur, (frame + 4) & 0xf);
			}
		}
		PlaceItem(&cur, Item_getX(cur) + dx, Item_getY(cur) + dy);
	}
	BargeAnimationDue = 0;
	return Item_move(&barge, x2 + dx, y2 + dy);
}

/* Can the barge turn to face dir? Only quarter and half turns, and only if its sides are both odd or both even. */
char far Barge_canTurn(objref &barge, unsigned char dir)
{
	Coord x1, y1, x2, y2, cx, cy, nx1, ny1, nx2, ny2;
	char ok;
	int turn, odd, width, height;

	turn = (dir - Barge_getDir(barge)) & 7;
	if (turn == 0 || turn == 4)
		return 1;
	if (turn & 1)
		return 0;
	width = Barge_getWidth(barge);
	height = Barge_getHeight(barge);
	if ((width & 1) && (height & 1))
		odd = 0;
	else if (!(width & 1) && !(height & 1))
		odd = 1;
	else
		return 0;
	x2 = Item_getX(barge);
	y2 = Item_getY(barge);
	x1 = Coord(x2.value - width + 1);
	y1 = Coord(y2.value - height + 1);
	cx = x1 + (x2 - x1) / 2;
	cy = y1 + (y2 - y1) / 2;
	nx1 = cx + (cy - y2) + odd;
	ny1 = cy + (x1 - cx);
	nx2 = cx + (cy - y1) + odd;
	ny2 = cy + (x2 - cx);
	ok = 1;
	if (nx2 > x2) {
		ok = Barge_isStripFree(barge, x2 + 1, ny1, nx2, ny2);
		if (ok)
			ok = Barge_isStripFree(barge, nx1, ny1, x1 - 1, ny2);
	} else if (nx2 < x2) {
		ok = Barge_isStripFree(barge, nx1, ny1, nx2, y1 - 1);
		if (ok)
			ok = Barge_isStripFree(barge, nx1, y2 + 1, nx2, ny2);
	}
	return ok;
}

/* Turns the barge to face dir, carrying and turning everything on its deck. */
char far Barge_turn(objref &barge, unsigned char dir)
{
	int turn;
	objref cur;
	TypeFrame frame;
	unsigned type;
	int count;
	int width, height;
	unsigned z;
	Coord x1, y1, x2, y2;
	Coord cx, cy;
	Coord nx1, ny1, nx2, ny2;
	int odd, i;
	AreaSearch items;

	width = Barge_getWidth(barge);
	height = Barge_getHeight(barge);
	if (width & 1 && height & 1)
		odd = 0;
	else if (!(width & 1) && !(height & 1))
		odd = 1;
	else
		return 0;
	x2 = Item_getX(barge);
	y2 = Item_getY(barge);
	x1 = Coord(x2.value - width + 1);
	y1 = Coord(y2.value - height + 1);
	z = Item_getZ(&barge);
	cx = x1 + (x2 - x1) / 2;
	cy = y1 + (y2 - y1) / 2;

	/* The deck's corners once turned about its centre. */
	turn = (dir - Barge_getDir(barge)) & 7;
	switch (turn) {
	case 0:
		return 1;
	case 2:
	case 6:
		nx1 = cx + (cy - y2) + odd;
		ny1 = cy + (x1 - cx);
		nx2 = cx + (cy - y1) + odd;
		ny2 = cy + (x2 - cx);
		break;
	case 4:
		nx1 = cx + (cx - x2) + odd;
		ny1 = cy + (cy - y2) + odd;
		nx2 = cx + (cx - x1) + odd;
		ny2 = cy + (cy - y1) + odd;
		break;
	}
	if (x1 < CellWindowX || x2 >= CellWindowX + CELL_WINDOW
		|| y1 < CellWindowY || y2 >= CellWindowY + CELL_WINDOW)
		return 0;
	if (nx1 < CellWindowX || nx2 >= CellWindowX + CELL_WINDOW
		|| ny1 < CellWindowY || ny2 >= CellWindowY + CELL_WINDOW)
		return 0;

	/* Lift everything on the deck off the map. */
	count = 0;
	FindArea(&items, x1, y1, x2, y2, FIND_NO_NPCS, -1, 255, 255);
	while (items.found()) {
		cur = items.current;
		type = ITEM_TYPE(cur.ptr());
		if (HasTypeClass(cur)
			&& ((unsigned char)gItemTypeInfo[type].solid || (unsigned char)gItemTypeInfo[type].bargePart)
			&& Item_getZ(&cur) >= z) {
			count++;
			FindItem(&items);
			Item_detach(&cur);
		} else
			FindItem(&items);
	}

	/* Put it back down turned, each piece on its new square. */
	switch (turn) {
	case 2:
		for (i = 0; i < count; i++) {
			GetItemBeingDragged(&cur);
			frame = cur.ptr()->typeFrame;
			cur.ptr()->typeFrame = RotateShapeQuarter(frame).bits;
			PlaceItem(&cur, cx + (cy - Item_getY(cur) + GetFootprintY(frame)) + odd,
				cy + (Item_getX(cur) - cx));
		}
		Barge_setWidth(barge, height);
		Barge_setHeight(barge, width);
		break;
	case 4:
		for (i = 0; i < count; i++) {
			GetItemBeingDragged(&cur);
			frame = cur.ptr()->typeFrame;
			cur.ptr()->typeFrame = RotateShapeHalf(frame).bits;
			PlaceItem(&cur, cx + (cx - Item_getX(cur) + GetFootprintX(frame)) + odd,
				cy + (cy - Item_getY(cur) + GetFootprintY(frame)) + odd);
		}
		break;
	case 6:
		for (i = 0; i < count; i++) {
			GetItemBeingDragged(&cur);
			frame = cur.ptr()->typeFrame;
			cur.ptr()->typeFrame = RotateShapeBack(frame).bits;
			PlaceItem(&cur, cx + (Item_getY(cur) - cy),
				cy + (cx - Item_getX(cur) + GetFootprintX(frame)) + odd);
		}
		Barge_setWidth(barge, height);
		Barge_setHeight(barge, width);
		break;
	}
	Barge_setDir(barge, dir);
	return Item_move(&barge, nx2, ny2);
}

/* Is the strip x1..x2, y1..y2 free for the barge: nothing in the way, and the right terrain under a boat or cart? */
unsigned char far Barge_isStripFree(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2)
{
	int width, height, z, mode;

	width = x2 - x1 + 1;
	height = y2 - y1 + 1;
	z = Item_getZ(&barge);
	if (IsBoxBlockedAt(x2, y2, z, width, height, 15))
		return 0;
	if (z != 0) {
		mode = 2;
		return 1;
	}
	return Barge_checkTerrain(barge, x1, y1, x2, y2, (mode = IsOnWater(barge), mode));
}

/* Checks the terrain under a strip: mode 0 wants no solid ground, mode 1 wants water everywhere. */
unsigned char far Barge_checkTerrain(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2, int mode)
{
	int i, j, width, height;
	int col, row;

	width = x2 - x1 + 1;
	height = y2 - y1 + 1;
	row = y2 - CellWindowY;
	for (j = 0; j < height; j++) {
		col = x2 - CellWindowX;
		for (i = 0; i < width; i++) {
			if (col < 0 || row < 0
				|| mode == 0 && (unsigned char)gItemTypeInfo[CellBuffer[row][col] & 0x3ff].solid
				|| mode == 1 && !(unsigned char)gItemTypeInfo[CellBuffer[row][col] & 0x3ff].water)
				return 0;
			col--;
		}
		row--;
	}
	return 1;
}

/* Raises the barge one level, with everything on its deck. */
char far Barge_rise(objref &barge)
{
	AreaSearch items;
	objref object;
	unsigned type, z;
	int rise;
	Coord x1, y1, x2, y2;
	int i, count;

	x2 = Item_getX(barge);
	y2 = Item_getY(barge);
	x1 = Coord(x2.value - Barge_getWidth(barge) + 1);
	y1 = Coord(y2.value - Barge_getHeight(barge) + 1);
	z = Item_getZ(&barge);
	count = 0;
	rise = 1;
	FindArea(&items, x1, y1, x2, y2, FIND_NO_NPCS, -1, 255, 255);
	while (items.found()) {
		object = items.current;
		type = ITEM_TYPE(object.ptr());
		if (HasTypeClass(object)
			&& ((unsigned char)gItemTypeInfo[type].solid || (unsigned char)gItemTypeInfo[type].bargePart)
			&& Item_getZ(&object) >= z) {
			if ((unsigned char)(Item_getZ(&object) + gItemTypeInfo[ITEM_TYPE(object.ptr())].height) > 15) {
				rise = 0;
				break;
			}
			count++;
			FindItem(&items);
			Item_detach(&object);
		} else
			FindItem(&items);
	}
	for (i = 0; i < count; i++) {
		GetItemBeingDragged(&object);
		PlaceItem(&object, Item_getX(object), Item_getY(object), Item_getZ(&object) + rise);
	}
	Item_setZ(&barge, Item_getZ(&barge) + rise);
	return rise > 0;
}

/* Lowers the barge one level, with everything on its deck. */
char far Barge_descend(objref &barge)
{
	AreaSearch items;
	int width, height;
	unsigned z;
	Coord x1, y1, x2, y2;
	objref object;
	unsigned type;
	int i, count;

	width = Barge_getWidth(barge);
	height = Barge_getHeight(barge);
	x2 = Item_getX(barge);
	y2 = Item_getY(barge);
	x1 = Coord(x2.value - width + 1);
	y1 = Coord(y2.value - height + 1);
	z = Item_getZ(&barge);
	if (z == 0)
		return 0;
	if (IsBoxBlockedAt(x2, y2, z - 1, width, height, 1))
		return 0;
	if (z == 1 && !Barge_checkTerrain(barge, x1, y1, x2, y2, 0))
		return 0;
	count = 0;
	FindArea(&items, x1, y1, x2, y2, FIND_NO_NPCS, -1, 255, 255);
	while (items.found()) {
		object = items.current;
		type = ITEM_TYPE(object.ptr());
		if (HasTypeClass(object)
			&& ((unsigned char)gItemTypeInfo[type].solid || (unsigned char)gItemTypeInfo[type].bargePart)
			&& Item_getZ(&object) >= z) {
			count++;
			FindItem(&items);
			Item_detach(&object);
		} else
			FindItem(&items);
	}
	for (i = 0; i < count; i++) {
		GetItemBeingDragged(&object);
		PlaceItem(&object, Item_getX(object), Item_getY(object), Item_getZ(&object) - 1);
	}
	Item_setZ(&barge, Item_getZ(&barge) - 1);
	return 1;
}

/* Is the barge free to stand where it is? */
char far Barge_isOkayToLand(objref &barge)
{
	AreaSearch items;
	int width, height, z;
	Coord x1, y1, x, y;

	width = Barge_getWidth(barge);
	height = Barge_getHeight(barge);
	x = Item_getX(barge);
	y = Item_getY(barge);
	x1 = Coord(x.value - width + 1);
	y1 = Coord(y.value - height + 1);
	z = Item_getZ(&barge);
	if (IsBoxBlockedAt(x, y, 0, width, height, z))
		return 0;
	if (!Barge_checkTerrain(barge, x1, y1, x, y, 0))
		return 0;
	return 1;
}

/* Finds the barge whose deck covers item, if any. */
objref far FindBargeUnder(objref item)
{
	AreaSearch items;
	objref object, barge;
	Coord x, y, left, top;

	x = Item_getX(item);
	y = Item_getY(item);
	barge = 0;
	for (FindArea(&items, x - 20, y - 20, x + 20, y + 20, FIND_EGGS, BARGE_TYPE, 255, 255);
		items.current.valid(); FindItem(&items)) {
		object = items.current;
		left = Item_getX(object);
		top = Item_getY(object);
		if (x <= left && y <= top
			&& x >= left - (Barge_getWidth(object) - 1)
			&& y >= top - (Barge_getHeight(object) - 1)) {
			barge = object;
			break;
		}
	}
	return barge;
}

/* Turns a shape a quarter turn one way, as its barge turns. */
TypeFrame far RotateShapeQuarter(TypeFrame typeFrame)
{
	unsigned type = typeFrame.type();

	if (IS_NPC(typeFrame.type()) && !(unsigned char)gItemTypeInfo[type].strangeMovement) {
		if (typeFrame.flipped())
			typeFrame.bits &= 0x7fff;
		else {
			typeFrame.setFrame(typeFrame.frame() ^ 0x10);
			typeFrame.bits |= 0x8000;
		}
	} else if (type == SEAT) {
		unsigned frame = typeFrame.frame();
		typeFrame.setFrame((frame & ~3) + ((frame + 1) & 3));
	} else if (typeFrame.flipped()) {
		if ((unsigned char)gItemTypeInfo[type].bargePart)
			typeFrame.setFrame(typeFrame.frame() ^ 3);
		typeFrame.bits &= 0x7fff;
	} else {
		if ((unsigned char)gItemTypeInfo[type].bargePart)
			typeFrame.setFrame(typeFrame.frame() ^ 1);
		typeFrame.bits |= 0x8000;
	}
	return typeFrame;
}

/* Turns a shape half round. */
TypeFrame far RotateShapeHalf(TypeFrame typeFrame)
{
	unsigned type = typeFrame.type();

	if (IS_NPC(typeFrame.type()) && !(unsigned char)gItemTypeInfo[type].strangeMovement)
		typeFrame.setFrame(typeFrame.frame() ^ 0x10);
	else if ((unsigned char)gItemTypeInfo[type].bargePart || type == SEAT)
		typeFrame.setFrame(typeFrame.frame() ^ 2);
	return typeFrame;
}

/* Turns a shape a quarter turn the other way. */
TypeFrame far RotateShapeBack(TypeFrame typeFrame)
{
	unsigned type = typeFrame.type();

	if (IS_NPC(typeFrame.type()) && !(unsigned char)gItemTypeInfo[type].strangeMovement) {
		if (typeFrame.flipped()) {
			typeFrame.setFrame(typeFrame.frame() ^ 0x10);
			typeFrame.bits &= 0x7fff;
		} else
			typeFrame.bits |= 0x8000;
	} else if (typeFrame.type() == SEAT) {
		unsigned frame = typeFrame.frame();
		typeFrame.setFrame((frame & ~3) + ((frame - 1) & 3));
	} else if (typeFrame.flipped()) {
		if ((unsigned char)gItemTypeInfo[type].bargePart)
			typeFrame.setFrame(typeFrame.frame() ^ 1);
		typeFrame.bits &= 0x7fff;
	} else {
		if ((unsigned char)gItemTypeInfo[type].bargePart)
			typeFrame.setFrame(typeFrame.frame() ^ 3);
		typeFrame.bits |= 0x8000;
	}
	return typeFrame;
}

/* Leaves the current vehicle, running the sails' usecode if it is a ship. */
void far LeaveVehicle()
{
	if (CurrentVehicle != 0) {
		if (CurrentVehicle != 1 && ITEM_TYPE(objref(CurrentVehicle).ptr()) == SAILS)
			RunUsable(1, CurrentVehicle, 0xffff);
		CurrentVehicle = 0;
		ActiveBarge = 0;
	}
}
