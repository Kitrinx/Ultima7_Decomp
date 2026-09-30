/* Black Gate U7.EXE, overlay segment 341 (file offsets 0x0a15f0 to 0x0a262e, 4158 bytes).
 * Borland C++ 2.0 -mm -O -P -Z -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "u7manage.h"
#include "item.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "itable.h"
#include "u7event.h"
#include "oops.h"
#include "bltshape.h"
#include "bogus.h"
#include "cast.h"
#include "itemovr1.h"
#include "text.h"
#include "use.h"
#include "type.h"
#include "npcref.h"

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define CLASS_FLAGS(p) (ItemTypeClassFlags[TYPE_CLASS(p)])

void *operator new(size_t);
void operator delete(void *);

struct View;

struct PointOffset { int16_t x, y; };
/* each kind of container's close button, from its corner */
const PointOffset CloseButtonOffsets[] = {
	{ 23, 42 },
	{ 23, 52 },
	{ 23, 63 },
	{ 23, 52 },
	{ 23, 44 },
	{ 0, 0 },
	{ 23, 51 },
	{ 23, 35 },
	{ 26, 115 },
	{ 23, 84 },
	{ 23, 36 },
};

struct RectangleOffset { int16_t x, y, x1, y1; };
/* each kind of container's contents area, from its corner */
const RectangleOffset ContainerContentAreas[] = {
	{ 46, 28, 124, 63 },
	{ 49, 19, 117, 61 },
	{ 50, 50, 120, 86 },
	{ 44, 40, 118, 79 },
	{ 43, 32, 119, 58 },
	{ 0, 0, 0, 0 },
	{ 52, 18, 139, 51 },
	{ 39, 20, 102, 58 },
	{ 22, 20, 90, 73 },
	{ 31, 11, 120, 91 },
	{ 42, 15, 109, 48 },
};

void ContainerGump::initialize()
{
	int16_t x = 0, y = 0;
	int16_t width, height;
	switch (TYPE(ITEM(displayed.off))) {
	case 799: kind = 0; shape = 1359; break;    /* unsealed box */
	case 802: kind = 1; shape = 1368; break;    /* bag */
	case 801: kind = 3; shape = 1369; break;    /* backpack */
	case 803: kind = 4; shape = 1370; break;    /* basket */
	case 804: kind = 6; shape = 1360; break;    /* crate */
	case 800: kind = 7; shape = 1381; break;    /* chest */
	case 819: kind = 8; shape = 1367; break;    /* barrel */
	case 405: kind = 9; shape = 1385; break;    /* ship's hold */
	case 283: case 406: case 407: case 416: case 679:   /* desk, nightstand, drawers */
		kind = 10; shape = 1386; break;
	default: kind = 2; shape = 1412; break;
	}
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	add(&contents);
	add(&closeButton);
	contents.initialize(displayed, x + ContainerContentAreas[kind].x,
		y + ContainerContentAreas[kind].y, x + ContainerContentAreas[kind].x1,
		y + ContainerContentAreas[kind].y1);
	closeButton.show();
	contents.show();
	int16_t mx = MouseState_getX(GetLastMouseState());
	int16_t my = GetLastMouseState()->y;
	if (mx + width > 319) mx = 319 - width;
	if (my + height > 199) my = 199 - height;
	moveTo(mx, my);
	Panel::show();
}

uint8_t ContainerGump::handle(MouseState *state)
{
	uint8_t result, hit;
	if (bounds.contains(MouseState_getX(state), state->y)) {
		if ((result = contents.handle(state)) != 0) return result;
		if ((result = closeButton.handle(state)) != 0) {
			switch (result) {
			case BUTTON_CLICKED: return GUMP_CLOSE;
			default: return result;
			}
		}
		hit = gShapeManager.isCursorInBounds(shape, 0, bounds,
			Point(MouseState_getX(state), state->y));
		if (hit != 0) {
			if (state->action == MOUSE_CLICK) return GUMP_MOVE;
			if (state->action == MOUSE_RELEASE) return GUMP_NO_DROP;
		}
	}
	return 0;
}

void ContainerGump::draw(View *target)
{
	if (visible) ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
}

void ContainerGump::moveTo(int16_t x, int16_t y)
{
	bounds.moveTo(x, y);
	closeButton.moveTo(x + CloseButtonOffsets[kind].x, y + CloseButtonOffsets[kind].y);
	contents.moveTo(x + ContainerContentAreas[kind].x, y + ContainerContentAreas[kind].y);
}

uint8_t ContainerGump::accepts(objref moved, int16_t x, int16_t y)
{
	objref owner;
	owner = GetOuterContainer(object(), moved);
	if (Item_isOkayToTake(&owner) || NPCRef(owner).isBody()) {
		MarkItemOkayToTake(moved);
	}
	if (!contents.accepts(moved, x - contents.offsetX, y - contents.offsetY)) return 0;
	return 1;
}

objref ContainerGump::selected() { return contents.selectedItem; }

int16_t ContainerGump::dragX() { return contents.offsetX; }

int16_t ContainerGump::dragY() { return contents.offsetY; }

void ContainerGump::setDragX(int16_t n) { contents.offsetX = n; }

void ContainerGump::setDragY(int16_t n) { contents.offsetY = n; }

int16_t ContainerGump::mouseX() { return contents.clickX; }

int16_t ContainerGump::mouseY() { return contents.clickY; }

void ContainerGump::refresh(int8_t force)
{
	if (dirty || force) {
		contents.refresh();
		dirty = 0;
	}
}

uint8_t ContainerGump::findPosition(objref item, int16_t *x, int16_t *y)
{
	ItemNode *node = 0;
	while (List_stepForward(&contents.items, (DoubleLink **)&node)) {
		if (node->object == item) {
			*x = contents.region.x + node->x;
			*y = contents.region.y + node->y;
			return 1;
		}
	}
	return 0;
}

void ItemList::append(objref item, uint8_t x, uint8_t y)
{
	ItemNode *node = new ItemNode;
	if (!node) ReportOutOfNearMemory();
	node->object = item;
	node->x = x;
	node->y = y;
	node->active = 1;
	List_insertAtTail(this, node);
}

void ItemList::prepend(objref item, uint8_t x, uint8_t y)
{
	ItemNode *node = new ItemNode;
	if (!node) ReportOutOfNearMemory();
	node->object = item;
	node->x = x;
	node->y = y;
	node->active = 1;
	List_insertAtHead(this, node);
}

void ItemList::markStale()
{
	ItemNode *node = 0;
	while (List_stepForward(this, (DoubleLink **)&node)) node->active = 0;
}

ItemNode *ItemList::find(objref item)
{
	ItemNode *node = 0;
	while (List_stepBackward(this, (DoubleLink **)&node)) {
		if (node->object == item) break;
	}
	return node;
}

void ItemList::removeStale()
{
	ItemNode *node = 0;
	while (List_stepForward(this, (DoubleLink **)&node)) {
		if (!node->active) {
			List_removeAndDestroy(this, node);
			node = 0;
		}
	}
}

objref ItemGrid::object()
{
	objref empty(0);
	return empty;
}

void ItemGrid::initialize(objref item, uint8_t x, uint8_t y, uint8_t x1, uint8_t y1)
{
	region.set(x, y, x1, y1);
	container = item;
	build(container);
}

void ItemGrid::refresh() { build(container); }

static int16_t column, row, rowHeight, inset;

void ItemGrid::place(objref item, uint8_t *x, uint8_t *y, uint8_t *reset)
{
	int16_t height, width;
	if (*reset) {
		inset = 0;
		column = rowHeight = 0;
		row = 0;
		*reset = 0;
	}
	gShapeManager.getFrameSize(&width, &height, TYPE(ITEM(item.off)), FRAME(ITEM(item.off)));
	if (region.width() <= width || region.height() <= height) {
		*x = width;
		*y = height;
	} else {
		for (;;) {
			*x = column + width + inset;
			*y = row + height + inset;
			if (region.width() < *x) {
				*x = width;
				row += rowHeight;
				*y = row + height;
				rowHeight = 0;
			}
			if (region.height() >= *y) break;
			row = 0;
			column = 0;
			rowHeight = 0;
			inset += 8;
			if (inset > region.height() || inset > region.width()) inset = 0;
		}
		if (*y > rowHeight) rowHeight = *y;
	}
	column = *x;
}

void ItemGrid::build(objref item)
{
	uint8_t x, y;
	objref current;
	ItemNode *node = 0;
	uint8_t reset = 1;
	container = item;
	current = GetContainedItem(&container);
	items.markStale();
	List_stepForward(&items, (DoubleLink **)&node);
	while (current.valid()) {
		if (node) {
			if (node->object == current) {
				Item_setQuantity(current, (uint8_t)Item_getQuantity(&current), 1);
				node->active = 1;
				place(node->object, &x, &y, &reset);
			} else {
				place(node->object, &x, &y, &reset);
				List_stepForward(&items, (DoubleLink **)&node);
				continue;
			}
			List_stepForward(&items, (DoubleLink **)&node);
		} else {
			place(current, &x, &y, &reset);
			items.append(current, x, y);
		}
		current = current.next();
	}
	items.removeStale();
}

ItemNode *ItemGrid::find(int16_t x, int16_t y)
{
	ItemNode *node = 0;
	while (List_stepBackward(&items, (DoubleLink **)&node)) {
		if (gShapeManager.isCursorInBounds(TYPE(ITEM(node->object.off)), FRAME(ITEM(node->object.off)),
			Point(region.x + node->x, region.y + node->y), Point(x, y))) return node;
	}
	return 0;
}

uint8_t ItemGrid::handle(MouseState *state)
{
	ItemNode *node;
	if (region.contains(MouseState_getX(state), state->y)) {
		if (state->action == MOUSE_CLICK || state->action == MOUSE_DOUBLE_CLICK) {
			node = find(MouseState_getX(state), state->y);
			if (node && node->object.valid()) {
				selectedItem = node->object;
				offsetX = MouseState_getX(state) - (region.x + node->x);
				offsetY = state->y - (region.y + node->y);
				if (PickingItem && state->action == MOUSE_CLICK) return GUMP_SELECT_ITEM;
				if (state->action == MOUSE_DOUBLE_CLICK) {
					if (Item_canBeOpened(selectedItem)) return GUMP_OPEN_ITEM;
					return GUMP_USE_ITEM;
				}
				clickX = MouseState_getX(state);
				clickY = state->y;
				if (WaitForClick(*state)) return GUMP_SELECT_ITEM;
				List_removeAndDestroy(&items, node);
				return GUMP_DRAG_ITEM;
			}
		} else if (state->action == MOUSE_RELEASE) return GUMP_DROP_HERE;
	}
	return 0;
}

void ItemGrid::moveTo(int16_t x, int16_t y) { region.moveTo(x, y); }

void ItemGrid::draw(View *target)
{
	ItemNode *node = 0;
	int16_t width, height;
	if (visible) {
		while (List_stepForward(&items, (DoubleLink **)&node)) {
			gShapeManager.getFrameSize(&width, &height, TYPE(ITEM(node->object.off)), FRAME(ITEM(node->object.off)));
			ShapeManager_drawItem(&gShapeManager, region.x + node->x, region.y + node->y, node->object, target);
		}
	}
}

uint8_t ItemGrid::accepts(objref moved, int16_t x, int16_t y)
{
	objref destination = container;
	ItemNode *node = 0;
	uint8_t nested = 0;
	int16_t result;
	node = find(x + offsetX, y + offsetY);
	if (node && node->object.valid()) {
		objref target = node->object;
		if (CanStackWith(&target, moved)) {
			Item_setQuantity(moved,
				(uint8_t)Item_getQuantity(&target) + (uint8_t)Item_getQuantity(&moved), 0);
			Item_setQuantity(target, 0, 0);
			PlaceDraggedInContainer(&moved, destination);
			List_unlink(&items, node);
			List_insertAtTail(&items, node);
			node->object = objref(moved.off);
			return 1;
		}
		if ((uint8_t)(CLASS_FLAGS(ITEM(target.off)) & CLASS_CONTENTS)) {
			destination = target;
			nested = 1;
		}
	}
	result = TryToPlaceItem(destination, nested, 0);
	switch (result) {
	case 0:
		if (container == destination) items.append(moved, x - region.x, y - region.y);
		return 1;
	case 1: ReportNoCanDo(4); break;
	case 2: case 3: ReportNoCanDo(0); break;
	case 4: ReportNoCanDo(5); break;
	}
	return 0;
}

ProportionalTextPrinter StatsTextPrinter;

extern "C" void ResetContgumpGlobals(void)
{
	column = 0;
	row = 0;
	rowHeight = 0;
	inset = 0;
	memset((void *)&StatsTextPrinter, 0, sizeof(StatsTextPrinter));
}

extern "C" void ConstructContgumpGlobals(void)
{
	new (&StatsTextPrinter) ProportionalTextPrinter();
}
