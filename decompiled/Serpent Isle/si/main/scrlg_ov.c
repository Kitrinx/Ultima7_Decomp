/* Serpent Isle SI.EXE, overlay segment 336 (file offsets 0x09d1f0 to 0x09d534, 836 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7manage.h"
#include "item.h"
#include "u7event.h"
#include "bltshape.h"
#include "gumps.h"

/* the scroll gump; its pictures follow it in gumps.vga */
#define SCROLL_SHAPE 1488

void SpellScrollGump::initialize()
{
	int height, width;

	shape = SCROLL_SHAPE;
	gShapeManager.getShapeSize(&width, &height, shape);
	bounds.set(0, 0, width, height);
	add(&closeButton);
	closeButton.show();
	Control::show();
	unsigned char quality = Item_getQuality(&displayed);
	picture.show();
	picture.setShape(quality / 8 + SCROLL_SHAPE + 1);
	picture.setFrame(quality % 8);
	add(&picture);
	moveTo(12, 12);
}

void SpellScrollGump::moveTo(int x, int y)
{
	bounds.moveTo(x, y);
	closeButton.moveTo(x + 20, y + 64);
	picture.moveTo(x + 33, y + 38);
}

unsigned char SpellScrollGump::handle(MouseState *event)
{
	unsigned char result = 0;

	if (!isVisible())
		return 0;
	int x = MouseState_getX(event);
	int y = event->y;
	if (!bounds.contains(x, y))
		return 0;
	int hit = gShapeManager.isCursorInBounds(shape, 0, bounds, Point(x, y));
	if (!hit)
		return 0;
	if ((result = closeButton.handle(event)) != 0) {
		switch (result) {
		case BUTTON_CLICKED:
			return GUMP_CLOSE;
		default:
			return 0;
		}
	} else if ((result = picture.handle(event)) != 0) {
		switch (result) {
		case BUTTON_DOUBLE_CLICK:
			return GUMP_READ_SCROLL;
		case BUTTON_CLICKED:
			return GUMP_NO_DROP;
		default:
			return 0;
		}
	} else {
		if (event->action == MOUSE_CLICK)
			return GUMP_MOVE;
		if (event->action == MOUSE_RELEASE)
			return GUMP_NO_DROP;
	}
	return 0;
}

void SpellScrollGump::draw(View *target)
{
	if (visible)
		ShapeManager_draw(&gShapeManager, target, bounds.x, bounds.y, shape, 0, 0, 0);
}

unsigned char SpellScrollGump::accepts(objref, int, int) { return 0; }

objref SpellScrollGump::selected()
{
	objref result = 0;

	return result;
}

int SpellScrollGump::mouseX() { return clickX; }

int SpellScrollGump::mouseY() { return clickY; }

int SpellScrollGump::dragX() { return dragOffsetX; }

int SpellScrollGump::dragY() { return dragOffsetY; }

void SpellScrollGump::setDragX(int x) { dragOffsetX = x; }

void SpellScrollGump::setDragY(int y) { dragOffsetY = y; }

void SpellScrollGump::refresh(char) {}

unsigned char SpellScrollGump::findPosition(objref, int *, int *) { return 0; }
