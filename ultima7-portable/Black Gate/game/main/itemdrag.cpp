/* Black Gate U7.EXE, overlay segment 343 (file offsets 0x0a4a90 to 0x0a4fb0, 1312 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "plat.h"
#include "lowlevel.h"
#include "u7manage.h"
#include "item.h"
#include "colbuf.h"
#include "gumps.h"
#include "u7event.h"
#include "bltshape.h"
#include "u7point.h"
#include "camera.h"

#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define FRAME(p) (((p)->typeFrame & 0x7c00) >> 10)

extern View Viewport;

void ItemDrag::hide()
{
	gShapeManager.restoreUnderShape(&Viewport, saved, lastX, lastY, TYPE(obj.ptr()), FRAME(obj.ptr()));
}

void ItemDrag::pickUp(int16_t x, int16_t y)
{
	int16_t w, h;

	gShapeManager.getShapeSize(&w, &h, TYPE(obj.ptr()));
	allocate(w * h);
	show(x, y);
	ShapeManager_drawItem(&gShapeManager, x - hotX, y - hotY, obj, &Viewport);
}

void ItemDrag::dragTo(int16_t x, int16_t y)
{
	int16_t dx = x - hotX;
	int16_t dy = y - hotY;

	hide();
	show(x, y);
	moveTo(dx, dy);
	ShapeManager_drawItem(&gShapeManager, dx, dy, obj, &Viewport);
	CopyFrameBuffer();
}

void ItemDrag::show(int16_t x, int16_t y)
{
	gShapeManager.saveUnderShape(&Viewport, saved, x - hotX, y - hotY, TYPE(obj.ptr()), FRAME(obj.ptr()));
	lastX = x - hotX;
	lastY = y - hotY;
}

void ItemDrag::drop(int16_t x, int16_t y)
{
	release();
	moveTo(x - hotX, y - hotY);
}

void ItemDrag::moveTo(int16_t x, int16_t y)
{
	posx = x;
	posy = y;
}

void ItemDrag::drag(MouseState *state)
{
	int16_t x, y;

	UpdateAndCopyMouseState(state);
	x = MouseState_getX(state);
	y = state->y;
	pickUp(x, y);
	dragTo(x, y);
	ShowCursor();
	do {
		plat_yield();
		if (state->action == MOUSE_MOVE)
			dragTo(MouseState_getX(state), state->y);
	} while (!UpdateAndCopyMouseState(state)->released());
	drop(MouseState_getX(state), state->y);
}

void Gump::drag(MouseState *state)
{
	int16_t x, y;

	x = MouseState_getX(state);
	y = state->y;
	pickUp(x, y);
	dragTo(x, y);
	ShowCursor();
	while (!UpdateAndCopyMouseState(state)->released()) {
		plat_yield();
		if (state->action == MOUSE_MOVE)
			dragTo(MouseState_getX(state), state->y);
	}
	drop(MouseState_getX(state), state->y);
}

/* Paints the gump at the origin of a view of its own size, keeps that as its picture, and puts
 * the gump back where it was. */
void Gump::pickUp(int16_t x, int16_t y)
{
	int16_t oldx = bounds.x;
	int16_t oldy = bounds.y;
	View *pic;
	int16_t buf;

	buf = gShapeManager.allocateView(0, 0, bounds.x1 - bounds.x - 1, bounds.y1 - bounds.y - 1);
	moveTo(0, 0);
	FillView(gShapeManager.lockView(buf), 0xff);
	pic = (View *) buf;
	paint(pic);
	image = MakeShapeFromImage(buf);
	gShapeManager.releaseBlock(buf);
	moveTo(oldx, oldy);
	allocate(bounds.width() * bounds.height());
	hotX = x - bounds.x;
	hotY = y - bounds.y;
	show(x, y);
	ShapeManager_drawInViewport(&gShapeManager, bounds.x, bounds.y, image, 0, 0, 0);
}

void Gump::dragTo(int16_t x, int16_t y)
{
	int16_t dx = x - hotX;
	int16_t dy = y - hotY;

	hide();
	show(x, y);
	moveTo(dx, dy);
	ShapeManager_drawInViewport(&gShapeManager, dx, dy, image, 0, 0, 0);
	CopyFrameBuffer();
}

void Gump::show(int16_t x, int16_t y)
{
	lastX = x - hotX;
	lastY = y - hotY;
	gShapeManager.saveUnderShape(&Viewport, saved, lastX, lastY, shape, 0);
}

void Gump::drop(int16_t x, int16_t y)
{
	release();
	gShapeManager.releaseBlock(image);
	moveTo(x - hotX, y - hotY);
}

void Gump::hide()
{
	gShapeManager.restoreUnderShape(&Viewport, saved, lastX, lastY, shape, 0);
}
