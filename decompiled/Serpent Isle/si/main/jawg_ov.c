/* Serpent Isle SI.EXE, overlay segment 332 (file offsets 0x09a730 to 0x09b1aa, 2682 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7manage.h"
#include "item.h"
#include "itemrec.h"
#include "type.h"
#include "u7event.h"
#include "bltshape.h"
#include "colbuf.h"
#include "gumps.h"
#include "gumpmgr.h"
#include "bogus.h"
#include "text.h"
#include "npcref.h"

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)

#define JAWBONE_SHAPE   1479    /* the gump; the teeth follow it in gumps.vga */
#define SERPENT_TOOTH   559

/* the jawbone's contents area, from its corner */
#define TEETH_LEFT      0
#define TEETH_TOP       0
#define TEETH_RIGHT     20
#define TEETH_BOTTOM    20

struct PointOffset { int x, y; };

/* where each tooth sits in the jawbone */
PointOffset far ToothOffsets[TOOTH_COUNT] = {
	{ 34, 19 }, { 32, 30 }, { 31, 37 }, { 31, 44 }, { 28, 52 }, { 31, 57 },
	{ 27, 66 }, { 31, 77 }, { 40, 82 }, { 50, 84 }, { 57, 80 }, { 63, 71 },
	{ 72, 69 }, { 70, 61 }, { 75, 50 }, { 82, 42 }, { 83, 36 }, { 87, 32 },
};

void ToothSlot::initialize(objref item)
{
	tooth = item;
	frame = 0;
	shape = 0;
	if (tooth.valid())
		setTooth(tooth);
}

unsigned char ToothSlot::handle(MouseState *event)
{
	unsigned char result = 0;
	int mouseX, mouseY;
	unsigned char hit;

	if (!visible)
		return 0;
	mouseX = MouseState_getX(event);
	mouseY = event->y;
	hit = 0;
	if (!tooth.valid() && event->action != MOUSE_RELEASE)
		return 0;
	if (tooth.valid())
		hit = gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mouseX, mouseY));
	if (hit && event->action == MOUSE_CLICK && PickingItem)
		return GUMP_SELECT_ITEM;
	if (event->action == MOUSE_DOUBLE_CLICK) {
		if (hit) {
			result = GUMP_USE_ITEM;
			return result;
		}
		return result;
	} else if (event->action == MOUSE_CLICK) {
		if (hit) {
			if (WaitForClick(*event))
				return GUMP_SELECT_ITEM;
			return GUMP_DRAG_ITEM;
		}
		return result;
	}
	return 0;
}

void ToothSlot::moveTo(int newX, int newY)
{
	x = newX;
	y = newY;
}

void ToothSlot::draw(View *target)
{
	if (visible && tooth.valid())
		ShapeManager_draw(&gShapeManager, target, x, y, shape, frame, 0, 0);
}

unsigned char ToothSlot::setTooth(objref item)
{
	if (!item.valid())
		return 0;
	tooth = item;
	shape = JAWBONE_SHAPE + 1;
	frame = tooth.frame();
	return 1;
}

void JawboneGump::initialize()
{
	int x = 0, y = 0;
	int width, height;

	shape = JAWBONE_SHAPE;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	Control::show();
	add(&closeButton);
	closeButton.show();
	contents.initialize(displayed, x + TEETH_LEFT, y + TEETH_TOP, x + TEETH_RIGHT, y + TEETH_BOTTOM);
	update(1);
	moveTo(0, 0);
}

/* Puts each tooth the jawbone holds in the place its frame names. */
void JawboneGump::update(unsigned char attach)
{
	objref tooth;
	char i;

	contents.build(displayed);
	Item_setFrame(&displayed, 0);
	for (i = 0; i < TOOTH_COUNT; i++) {
		tooth = 0;
		DoubleLink *node = 0;
		while (List_stepForward(&contents.items, &node)) {
			if (((ItemNode *)node)->object.valid() && ((ItemNode *)node)->object.frame() == i) {
				tooth = ((ItemNode *)node)->object;
				Item_setFrame(&displayed, displayed.frame() + 1);
				break;
			}
		}
		slots[i].initialize(tooth);
	}
	for (i = 9; i < TOOTH_COUNT; i++) {
		if (attach)
			add(&slots[i]);
		slots[i].show();
	}
	for (i = 8; i >= 0; i--) {
		if (attach)
			add(&slots[i]);
		slots[i].show();
	}
}

void JawboneGump::refresh(char force)
{
	if (dirty || force) {
		update(0);
		dirty = 0;
	}
}

unsigned char JawboneGump::handle(MouseState *event)
{
	unsigned char result, hit;
	char i;
	int x = MouseState_getX(event);
	int y = event->y;

	if (!bounds.contains(x, y))
		return 0;
	if (!(hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y))))
		return 0;
	if (event->action == MOUSE_RELEASE)
		return GUMP_DROP_HERE;
	for (i = 0; i < TOOTH_COUNT; i++) {
		result = slots[i].handle(event);
		if (result != 0) {
			selectedItem = slots[i].tooth;
			int height, width;
			gShapeManager.getFrameSize(&width, &height,
				TYPE(ITEM(selectedItem.off)), FRAME(ITEM(selectedItem.off)));
			dragOffsetX = -width >> 1;
			dragOffsetY = -height >> 1;
			clickX = x;
			clickY = y;
			if (result == GUMP_DRAG_ITEM && !PickingItem) {
				objref tooth = 0;
				slots[i].tooth = tooth;
				DoubleLink *node = 0;
				while (List_stepForward(&contents.items, &node)) {
					if (((ItemNode *)node)->object.valid() && ((ItemNode *)node)->object.frame() == i) {
						tooth = ((ItemNode *)node)->object;
						break;
					}
				}
				List_removeAndDestroy(&contents.items, node);
				return result;
			}
			return result;
		}
	}
	if ((result = closeButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED: return GUMP_CLOSE;
		default: return result;
		}
	}
	return hit && event->action == MOUSE_CLICK ? GUMP_MOVE : 0;
}

void JawboneGump::draw(View *target)
{
	if (visible)
		ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
}

void JawboneGump::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	closeButton.moveTo(x + 26, y + 105);
	PointOffset far *positions = ToothOffsets;
	for (int i = 0; i < TOOTH_COUNT; i++)
		slots[i].moveTo(x + positions[i].x, y + positions[i].y);
}

/* A tooth's frame names its place in the jawbone. */
unsigned char JawboneGump::toothSlot(unsigned char *slot, objref tooth)
{
	*slot = tooth.frame();
	return 1;
}

unsigned char JawboneGump::accepts(objref incoming, int, int)
{
	unsigned char slot;

	if (incoming.type() != SERPENT_TOOTH) {
		ReportNoCanDo(0);
		return 0;
	}
	if (!toothSlot(&slot, incoming)) {
		ReportNoCanDo(0);
		return 0;
	}
	objref outer;
	outer = GetOuterContainer(object(), incoming);
	if (Item_isOkayToTake(&outer) || NPCRef(outer).isBody()) {
		MarkItemOkayToTake(incoming);
	}
	Item_clearTemporary(&incoming);
	if (NPCRef(outer).isBody() && !CanCarryWeight(NPCRef(outer), incoming)) {
		ReportNoCanDo(4);
		return 0;
	}
	slots[slot].setTooth(incoming);
	if (!contents.accepts(incoming, 2, 2))
		return 0;
	dirty = 1;
	return 1;
}

objref JawboneGump::selected() { return selectedItem; }

int JawboneGump::dragX() { return dragOffsetX; }

int JawboneGump::dragY() { return dragOffsetY; }

void JawboneGump::setDragX(int x) { dragOffsetX = x; }

void JawboneGump::setDragY(int y) { dragOffsetY = y; }

int JawboneGump::mouseX() { return clickX; }

int JawboneGump::mouseY() { return clickY; }

unsigned char JawboneGump::findPosition(objref item, int *x, int *y)
{
	for (char i = 0; i < TOOTH_COUNT; i++) {
		if (slots[i].tooth == item) {
			*x = slots[i].x;
			*y = slots[i].y;
			return 1;
		}
	}
	return 0;
}
