/* Black Gate U7.EXE, overlay segment 338 (file offsets 0x09dff0 to 0x09e7fd, 2061 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "worldgmp.h"
#include "iteminfo.h"
#include "u7manage.h"
#include "item.h"
#include "npcref.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "camera.h"
#include "partymov.h"
#include "mouse.h"
#include "u7event.h"
#include "cast.h"
#include "crime.h"
#include "itemovr1.h"
#include "text.h"
#include "sortitem.h"
#include "target.h"
#include "use.h"
#include "weight.h"
#include "type.h"
#include "coord.h"
#include "bogus.h"
#include "legalmov.h"
#include "u7map.h"

struct Rect : Point {
	int x1, y1;
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	int contains(int px, int py) { return (px >= x) & (px <= x1) & (py >= y) & (py <= y1); }
};

unsigned char HackMoverEnabled = 0;

#define TYPE(r) ((r).ptr()->typeFrame & 0x3ff)
#define TYPE_CLASS(r) (gItemTypeInfo[TYPE(r)].typeClass)
#define IS_VALID(r) ((unsigned char) ((r).off != 0))
#define IS_CLASS(r, f) ((unsigned char) (TYPE_CLASS(r) == (f)))

objref WorldGump::object()
{
	objref r(0);

	return r;
}

objref WorldGump::selected()
{
	return obj;
}

int WorldGump::mouseX()
{
	return sx;
}

int WorldGump::mouseY()
{
	return sy;
}

int WorldGump::dragX()
{
	return ox;
}

int WorldGump::dragY()
{
	return oy;
}

void WorldGump::setDragX(int v)
{
	ox = v;
}

void WorldGump::setDragY(int v)
{
	oy = v;
}

/* Pick up, open or look at what lies under the mouse. */
unsigned char WorldGump::handle(MouseState *state)
{
	int weight;
	int mx, my;

	FindItemAtScreenPoint(&obj, MouseState_getX(state), state->y, &MainWorldView, &gShapeManager, 1);
	if (!IS_VALID(obj)) {
		if (PickingItem && state->action == MOUSE_CLICK)
			return GUMP_PICK_GROUND;
		return GUMP_HANDLED;
	}
	if (state->action == MOUSE_DOUBLE_CLICK) {
		if (Item_canBeOpened(obj))
			return GUMP_OPEN_ITEM;
		return GUMP_USE_ITEM;
	}
	if (state->action == MOUSE_CLICK) {
		int family;
		Coord x, y;
		int z;

		mx = MouseState_getX(state);
		my = state->y;
		if (PickingItem || WaitForClick(*state))
			return GUMP_SELECT_ITEM;
		SelectMouseCursor(0);
		family = TYPE_CLASS(obj);
		/* type classes that cannot be picked up: unusable, eggs, barges, people and monsters */
		if (!HackMoverEnabled && (family == 7 || family == 0 || family == 9 || family == 13 || family == 12))
			return GUMP_HANDLED;
		z = Item_getZ(&obj);
		x = Item_getX(obj);
		y = Item_getY(obj);
		WorldCoordsToScreen(x, y, &sx, &sy);
		sx -= z * 4;
		sy -= z * 4;
		ox = mx - sx;
		oy = my - sy;
		weight = Item_getWeight(obj);
		if (!HackMoverEnabled) {
			if (weight == 0) {
				ReportNoCanDo(4);
				return GUMP_HANDLED;
			}
			if (!CanAvatarReach(obj, 0)) {
				ReportNoCanDo(7);
				return GUMP_HANDLED;
			}
		}
		return GUMP_DRAG_ITEM;
	}
	if (state->action == MOUSE_RELEASE)
		return GUMP_DROP_HERE;
	/* other events fall off the end and return the 1 left in AL */
}

void WorldGump::draw(View *)
{
	gCamera.beginFrame();
}

void WorldGump::moveTo(int, int)
{
}

/* Drop the dragged item at screen x, y: into what lies there, or onto the ground. */
unsigned char WorldGump::accepts(objref dragged, int x, int y)
{
	Coord wx, wy;
	int wz;
	objref target;
	unsigned char moved = 0;

	FindItemAtScreenPoint(&target, x, y, &MainWorldView, &gShapeManager, 1);
	if (IS_VALID(target) && !IS_CLASS(target, TYPE_CLASS_HUMAN) && !IS_CLASS(target, TYPE_CLASS_MONSTER)
		&& !CanStackWith(&target, dragged)) {
		int unusedType = TYPE(target);
		int rise = (unsigned char) (Item_getZ(&target) + gItemTypeInfo[TYPE(target)].height) * 4;
		int w = GetFootprintX(*(TypeFrame far *)&target.ptr()->typeFrame);
		int h = GetFootprintY(*(TypeFrame far *)&target.ptr()->typeFrame);
		int tx, ty;

		WorldCoordsToScreen(Item_getX(target), Item_getY(target), &tx, &ty);
		int x0 = tx - w * 8 - rise - 8;
		int y0 = ty - h * 8 - rise - 8;
		int x1 = tx - rise;
		int y1 = ty - rise;
		Rect r(x0, y0, x1, y1);
		if (!r.contains(x, y)) {
			x -= ox;
			y -= oy;
			target = 0;
			moved = 1;
		}
	}
	PickWorldCoordsOnItem(x, y, &wx, &wy, &wz, &target, 1);
	if (IS_VALID(target)) {
		if (!CanAvatarReach(target, 0)) {
			ReportNoCanDo(7);
			return 0;
		}
		unsigned char bag = IS_CLASS(dragged, TYPE_CLASS_QUANTITY);
		if (IS_CLASS(target, TYPE_CLASS_HUMAN) || IS_CLASS(target, TYPE_CLASS_MONSTER)) {
			if (!IsPartyMember(&NPCRef(target)))
				wz--;
			else {
				MarkItemOkayToTake(dragged);
				int result = TryToPlaceItem(target, 1, 1);
				switch (result) {
				case 0:
					return 1;
				case 1:
					ReportNoCanDo(4);
					break;
				case 2:
				case 3:
					ReportNoCanDo(0);
					break;
				case 4:
					ReportNoCanDo(5);
					break;
				}
				return 0;
			}
		} else if (CanStackWith(&target, dragged)) {
			if (Item_isOkayToTake(&target) != Item_isOkayToTake(&dragged)) {
				objref t;

				if (Item_isOkayToTake(&target))
					t = dragged;
				else
					t = target;
				MarkItemOkayToTake(t);
			}
			Item_setQuantity(target,
				(unsigned char)Item_getQuantity(&target) + (unsigned char)Item_getQuantity(&dragged), 0);
			ZapDetachedItem(&dragged);
			return 1;
		}
	}
	int dz = wz * 4;
	if (!moved) {
		x -= ox;
		y -= oy;
	}
	ScreenToWorldCoords(x + dz, y + dz, &wx.value, &wy.value, 1);
	char ok = CanItemMoveTo(wx, wy, (char) wz, dragged);
	if (ok || HackMoverEnabled) {
		PlaceItem(&dragged, wx, wy);
		Item_setZ(&dragged, wz);
		if (!CanAvatarReach(dragged, 0)) {
			Item_detach(&dragged);
			ReportNoCanDo(7);
			return 0;
		}
		if (!Item_isOkayToTake(&dragged) && !HackMoverEnabled && !TheftReported) {
			TheftReported = 1;
			ReportCrime(dragged.off, 0, 1);
			return 1;
		}
		return 1;
	}
	ReportNoCanDo(0);
	return 0;
}
