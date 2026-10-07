/* Serpent Isle MAINMENU.EXE, resident segment 8 (file offsets 0x00c769 to 0x00dbf2, 5257 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdlib.h>
#include "plat.h"
#include "dosio.h"
#include "lowlevel.h"
#include "u7manage.h"
#include "u7event.h"
#include "u7point.h"
#include "oops.h"
#include "controls.h"

namespace MainMenu {

#define SIGN(n)         ((n) < 0 ? -1 : 1)

void *ControlNode::operator new(size_t)
{
	ControlNode *node = ::new ControlNode;

	if (!node)
		ReportOutOfNearMemory();
	return node;
}

void ControlList::append(Control *child)
{
	ControlNode *node = new ControlNode(child);

	node->child = child;
	List_insertAtTail(this, node);
}

void ControlList::prepend(Control *child)
{
	ControlNode *node = new ControlNode(child);

	node->child = child;
	List_insertAtHead(this, node);
}

Control::Control()
{
	drawn = 0;
	parent = 0;
	visible = 1;
	id = 0;
}

Control::Control(Screen *screen)
{
	drawn = 0;
	parent = 0;
	visible = 1;
	id = 0;
	attach(screen);
}

/* Puts back what lay under the control; its screen has already let go of it. */
void Control::leave()
{
	if (visible && drawn)
		restoreUnder();
	parent = 0;
}

void Control::setParent(Screen *screen)
{
	if (parent)
		parent->remove(this);
	parent = screen;
	*(View *) this = *screen;
}

void Control::detach()
{
	if (visible && drawn)
		restoreUnder();
	if (parent) {
		parent->remove(this);
		parent = 0;
	}
}

void Control::attach(Screen *screen)
{
	if (parent)
		parent->remove(this);
	parent = screen;
	*(View *) this = *screen;
	parent->append(this);
}

void Control::setHighlight(uint8_t on)
{
	highlighted = on;
}

uint8_t Control::handleKey(int16_t)
{
	return 0;
}

uint8_t Control::press(int16_t, int16_t)
{
	return 0;
}

uint8_t Control::release(int16_t, int16_t)
{
	return 0;
}

int16_t Control::isDrawn()
{
	return drawn;
}

int16_t Control::isVisible()
{
	return visible;
}

void Control::show()
{
	visible = 1;
}

void Control::hide()
{
	visible = 0;
	drawn = 0;
}

int16_t Control::getId()
{
	return id;
}

void Control::setId(int16_t n)
{
	id = n;
}

Sprite::Sprite()
{
	keepUnder = 0;
	drawn = 0;
	x = 0;
	y = 0;
	scale = 256;
	backFrame = 0;
	visible = 1;
	highlighted = 0;
	drawFlags = 0;
}

Sprite::Sprite(char *name, int8_t back, Screen *screen, int8_t keep)
{
	load(name, screen, keep, back);
}

Sprite::Sprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
{
	load(flexName, entry, screen, keep, back);
}

Sprite::Sprite(void *data, int8_t back, Screen *screen, int8_t keep)
{
	init(data, screen, keep, back);
}

Sprite::~Sprite()
{
	detach();
}

void Sprite::load(char *name, Screen *screen, int8_t keep, int8_t back)
{
	keepUnder = keep;
	drawFlags = 0;
	shape.load(name);
	width = GetMaxFrameWidth(shape.linear(), drawFlags);
	height = GetMaxFrameHeight(shape.linear(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount(shape.linear(), drawFlags);
	frame = 0;
	drawn = 0;
	scale = 256;
	angle = 0;
	backFrame = back;
	parent = screen;
	if (screen)
		attach(screen);
}

void Sprite::load(char *flexName, int16_t entry, Screen *screen, int8_t keep, int8_t back)
{
	keepUnder = keep;
	drawFlags = 0;
	shape.load(flexName, entry);
	width = GetMaxFrameWidth(shape.linear(), drawFlags);
	height = GetMaxFrameHeight(shape.linear(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount(shape.linear(), drawFlags);
	frame = 0;
	drawn = 0;
	scale = 256;
	angle = 0;
	backFrame = back;
	parent = screen;
	if (screen)
		attach(screen);
}

/* Takes a shape already in memory, which the sprite does not own. */
void Sprite::init(void *data, Screen *screen, int8_t keep, int8_t back)
{
	keepUnder = keep;
	drawFlags = 0x111;
	shape.data = data;
	width = GetMaxFrameWidth(shape.linear(), drawFlags);
	height = GetMaxFrameHeight(shape.linear(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount(shape.linear(), drawFlags);
	frame = 0;
	drawn = 0;
	scale = 256;
	angle = 0;
	backFrame = back;
	parent = screen;
	if (screen)
		attach(screen);
}

void Sprite::saveUnder()
{
	if (keepUnder) {
		SaveUnderFrame(this, under.linear(), x, y, shape.linear(), frame, drawFlags);
		underFrame = frame;
		underX = x;
		underY = y;
		drawn = 1;
	}
}

void Sprite::draw()
{
	if (backFrame == 1)
		DrawFrame((View *) this, x, y, shape.linear(), 0, drawFlags);
	DrawFrame((View *) this, x, y, shape.linear(), frame, drawFlags);
}

void Sprite::restoreUnder()
{
	if (keepUnder)
		RestoreUnderFrame(this, under.linear(), underX, underY, shape.linear(), underFrame, drawFlags);
}

uint8_t Sprite::contains(int16_t px, int16_t py)
{
	Rect bounds;

	GetFrameBounds(&bounds, x, y, shape.linear(), frame, drawFlags);
	return (px >= bounds.x) & (px <= bounds.x1) & (py >= bounds.y) & (py <= bounds.y1);
}

void Sprite::nextFrame()
{
	frame++;
	setFrame(frame %= frameCount);
	if (backFrame == 1 && frame == 0)
		frame = 1;
}

uint8_t Sprite::stepForward()
{
	if (frame < frameCount - 1) {
		frame++;
		return 1;
	}
	return 0;
}

uint8_t Sprite::stepBackward()
{
	if (frame != 0) {
		frame--;
		return 1;
	}
	return 0;
}

void Sprite::setFrame(int16_t n)
{
	if (n >= 0 && n < frameCount)
		frame = n;
}

void Sprite::moveTo(int16_t nx, int16_t ny)
{
	x = nx;
	y = ny;
}

void Sprite::moveBy(int16_t dx, int16_t dy)
{
	x += dx;
	y += dy;
}

void Sprite::setScale(int16_t n)
{
	scale = n;
}

void Sprite::addScale(int16_t n)
{
	scale += n;
}

void Sprite::setAngle(int16_t n)
{
	angle = n;
}

void Sprite::turn(int16_t degrees)
{
	angle = (angle + degrees) % 360;
}

int16_t Sprite::getScale()
{
	return scale;
}

int16_t Sprite::lastFrame()
{
	return frameCount - 1;
}

void *Sprite::getData()
{
	return shape.get();
}

Screen::Screen()
{
	*(View *) this = Viewport;
	FillView((View *) this, 0);
}

Screen::Screen(int16_t color)
{
	*(View *) this = Viewport;
	FillView((View *) this, color);
}

void Screen::drawShape(char *name, int16_t frame, int16_t x, int16_t y)
{
	if (name) {
		FarBuffer shape(name);
		DrawFrame(&Viewport, x, y, shape.linear(), frame, 0);
	}
}

void Screen::drawShape(int16_t entry, char *flexName, int16_t frame, int16_t x, int16_t y)
{
	if (flexName) {
		FarBuffer shape(flexName, entry);
		DrawFrame(&Viewport, x, y, shape.linear(), frame, 0);
	}
}

void Screen::add(Control *child)
{
	controls.append(child);
	child->setParent(this);
}

void Screen::append(Control *child)
{
	controls.append(child);
}

/* Takes a control off the screen and puts back what lay under it. */
void Screen::drop(Control *child)
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node)) {
		if (((ControlNode *) node)->get() == child) {
			List_unlink(&controls, node);
			((ControlNode *) node)->get()->leave();
			break;
		}
	}
}

void Screen::remove(Control *child)
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node)) {
		if (((ControlNode *) node)->get() == child) {
			List_unlink(&controls, node);
			break;
		}
	}
}

void Screen::clear()
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node))
		((ControlNode *) node)->get()->leave();
	List_removeAndDestroyAll(&controls);
}

/* Redraws every visible control, over a fresh fill of the color or, with -1, over what they saved.
 * Unless told not to, the result is copied to the screen with the mouse cursor on it. */
void Screen::paint(int16_t color, uint8_t noCopy)
{
	DoubleLink *node = 0;
	Control *child;

	if (color == -1) {
		while (List_stepForward(&controls, &node))
			if (((ControlNode *) node)->get()->isDrawn()) {
				child = ((ControlNode *) node)->get();
				child->restoreUnder();
			}
		node = 0;
		while (List_stepForward(&controls, &node))
			if (((ControlNode *) node)->get()->isVisible()) {
				child = ((ControlNode *) node)->get();
				child->saveUnder();
			}
	} else
		FillView((View *) this, color);
	node = 0;
	while (List_stepForward(&controls, &node))
		if (((ControlNode *) node)->get()->isVisible()) {
			child = ((ControlNode *) node)->get();
			child->draw();
		}
	if (!noCopy) {
		uint8_t cursor;

		/* a host-drawn pointer stays out of the picture */
		if ((cursor = IsCursorDrawn() && !plat_cursor_is_hardware()) != 0) {
			DisableCursorTracking();
			DrawCursorInto(&Viewport);
		}
		CopyView(&Viewport, &ScreenView);
		if (cursor) {
			EraseCursorFrom(&Viewport);
			EnableCursorTracking();
		}
	}
}

BlendSprite::BlendSprite(void *data, int8_t back, Screen *screen, int8_t keep) : Sprite(data, back, screen, keep)
{
	table = 0;
	highlightTable = 0;
}

void BlendSprite::setTable(void *t)
{
	table = t;
}

void BlendSprite::setHighlightTable(void *t)
{
	highlightTable = t;
}

void BlendSprite::draw()
{
	void *blend = table;

	if (highlighted)
		blend = highlightTable;
	if (backFrame == 1)
		DrawFrameTranslucent(this, x, y, shape.linear(), 0, PointerToLinear(blend), drawFlags);
	DrawFrameTranslucent(this, x, y, shape.linear(), frame, PointerToLinear(blend), drawFlags);
}

Button::Button(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	align = ALIGN_LEFT;
}

void Button::draw()
{
	int16_t left = x;
	int16_t top = y;

	switch (align) {
	case ALIGN_CENTRE:
		left -= width >> 1;
		break;
	case ALIGN_RIGHT:
		left -= width;
		break;
	}
	if (backFrame == 1)
		DrawFrame((View *) this, left, top, shape.linear(), 0, drawFlags);
	int16_t shown = highlighted ? 1 : 0;
	DrawFrame((View *) this, left, top, shape.linear(), shown, drawFlags);
}

uint8_t Button::contains(int16_t px, int16_t py)
{
	int16_t left = x;
	int16_t top = y;

	switch (align) {
	case ALIGN_CENTRE:
		left -= width >> 1;
		break;
	case ALIGN_RIGHT:
		left -= width;
		break;
	}
	Rect bounds;
	GetFrameBounds(&bounds, left, top, shape.linear(), frame, drawFlags);
	return (px >= bounds.x) & (px <= bounds.x1) & (py >= bounds.y) & (py <= bounds.y1);
}

MovingSprite::MovingSprite(char *name, int8_t back, Screen *screen, int8_t keep) : Sprite(name, back, screen, keep)
{
	startX = startY = endX = endY = steps = 0;
}

MovingSprite::MovingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	startX = startY = endX = endY = steps = 0;
}

void MovingSprite::setPath(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t count)
{
	startX = x0;
	startY = y0;
	endX = x1;
	endY = y1;
	steps = count;
	step = 0;
	errorX = errorY = 0;
	x = startX;
	y = startY;
}

void MovingSprite::slideBy(int16_t dx, int16_t dy, int16_t count)
{
	setPath(x, y, x + dx, y + dy, count);
}

/* One step along the line; the remainders of the division carry over as in Bresenham's. */
void MovingSprite::advance()
{
	if (step <= steps) {
		step++;
		x += (endX - startX) / steps;
		y += (endY - startY) / steps;
		errorX += (endX - startX) % steps;
		errorY += (endY - startY) % steps;
		if (abs(errorX) > steps) {
			x += SIGN(errorX);
			errorX -= SIGN(errorX) * steps;
		}
		if (abs(errorY) > steps) {
			y += SIGN(errorY);
			errorY -= SIGN(errorY) * steps;
		}
	}
}

void MovingSprite::finish()
{
	x = endX;
	y = endY;
	step = steps;
}

BouncingSprite::BouncingSprite(char *name, int8_t back, Screen *screen, int8_t keep)
	: MovingSprite(name, back, screen, keep)
{
}

BouncingSprite::BouncingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	: MovingSprite(flexName, entry, back, screen, keep)
{
}

void BouncingSprite::setSpeed(int16_t dx, int16_t dy)
{
	speedX = dx;
	speedY = dy;
}

/* Drifts at random; speeds are in thirds of a pixel. */
void BouncingSprite::wander()
{
	int16_t dx = random(7) - 3;
	int16_t dy = random(7) - 3;

	if (x < 0) {
		speedX = 0;
		dx = 6;
	}
	if (x > SCREEN_WIDTH - 1 - width) {
		speedX = 0;
		dx = -6;
	}
	if (y < 0) {
		speedY = 0;
		dy = 6;
	}
	if (y > SCREEN_HEIGHT - 1 - height) {
		speedY = 0;
		dy = -6;
	}
	moveBy((speedX += dx) / 3, (speedY += dy) / 3);
}

/* Steps the frame back and forth between first and last. */
void BouncingSprite::cycleFrames(int16_t first, int16_t last)
{
	if (frame == last)
		frameStep = -1;
	else if (frame == first)
		frameStep = 1;
	frame += frameStep;
}

}
