/* Serpent Isle MAINMENU.EXE, resident segment 8 (file offsets 0x00c769 to 0x00dbf2, 5257 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "u7manage.h"
#include "u7event.h"
#include "u7point.h"
#include "oops.h"
#include "controls.h"

#define SIGN(n)         ((n) < 0 ? -1 : 1)

void *ControlNode::operator new(unsigned)
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
	copy(screen);
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
	copy(screen);
	parent->append(this);
}

void Control::setHighlight(unsigned char on)
{
	highlighted = on;
}

unsigned char Control::handleKey(int)
{
	return 0;
}

unsigned char Control::press(int, int)
{
	return 0;
}

unsigned char Control::release(int, int)
{
	return 0;
}

int Control::isDrawn()
{
	return drawn;
}

int Control::isVisible()
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

int Control::getId()
{
	return id;
}

void Control::setId(int n)
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

Sprite::Sprite(char *name, char back, Screen *screen, char keep)
{
	load(name, screen, keep, back);
}

Sprite::Sprite(char *flexName, int entry, char back, Screen *screen, char keep)
{
	load(flexName, entry, screen, keep, back);
}

Sprite::Sprite(void far *data, char back, Screen *screen, char keep)
{
	init(data, screen, keep, back);
}

Sprite::~Sprite()
{
	detach();
}

void Sprite::load(char *name, Screen *screen, char keep, char back)
{
	keepUnder = keep;
	drawFlags = 0;
	shape.load(name);
	width = GetMaxFrameWidth((long) shape.get(), drawFlags);
	height = GetMaxFrameHeight((long) shape.get(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount((long) shape.get(), drawFlags);
	frame = 0;
	drawn = 0;
	scale = 256;
	angle = 0;
	backFrame = back;
	parent = screen;
	if (screen)
		attach(screen);
}

void Sprite::load(char *flexName, int entry, Screen *screen, char keep, char back)
{
	keepUnder = keep;
	drawFlags = 0;
	shape.load(flexName, entry);
	width = GetMaxFrameWidth((long) shape.get(), drawFlags);
	height = GetMaxFrameHeight((long) shape.get(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount((long) shape.get(), drawFlags);
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
void Sprite::init(void far *data, Screen *screen, char keep, char back)
{
	keepUnder = keep;
	drawFlags = 0x111;
	shape.data = data;
	width = GetMaxFrameWidth((long) shape.get(), drawFlags);
	height = GetMaxFrameHeight((long) shape.get(), drawFlags);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	highlighted = 0;
	frameCount = GetShapeFrameCount((long) shape.get(), drawFlags);
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
		SaveUnderFrame(this, (long) under.get(), x, y, (long) shape.get(), frame, drawFlags);
		underFrame = frame;
		underX = x;
		underY = y;
		drawn = 1;
	}
}

void Sprite::draw()
{
	if (backFrame == 1)
		DrawFrame(this, x, y, (long) shape.get(), 0, drawFlags);
	DrawFrame(this, x, y, (long) shape.get(), frame, drawFlags);
}

void Sprite::restoreUnder()
{
	if (keepUnder)
		RestoreUnderFrame(this, (long) under.get(), underX, underY, (long) shape.get(), underFrame, drawFlags);
}

unsigned char Sprite::contains(int px, int py)
{
	Rect bounds;

	GetFrameBounds(&bounds, x, y, (long) shape.get(), frame, drawFlags);
	return bounds.contains(px, py);
}

void Sprite::nextFrame()
{
	frame++;
	setFrame(frame %= frameCount);
	if (backFrame == 1 && frame == 0)
		frame = 1;
}

unsigned char Sprite::stepForward()
{
	if (frame < frameCount - 1) {
		frame++;
		return 1;
	}
	return 0;
}

unsigned char Sprite::stepBackward()
{
	if (frame != 0) {
		frame--;
		return 1;
	}
	return 0;
}

void Sprite::setFrame(int n)
{
	if (n >= 0 && n < frameCount)
		frame = n;
}

void Sprite::moveTo(int nx, int ny)
{
	x = nx;
	y = ny;
}

void Sprite::moveBy(int dx, int dy)
{
	x += dx;
	y += dy;
}

void Sprite::setScale(int n)
{
	scale = n;
}

void Sprite::addScale(int n)
{
	scale += n;
}

void Sprite::setAngle(int n)
{
	angle = n;
}

void Sprite::turn(int degrees)
{
	angle = (angle + degrees) % 360;
}

int Sprite::getScale()
{
	return scale;
}

int Sprite::lastFrame()
{
	return frameCount - 1;
}

void far *Sprite::getData()
{
	return shape.get();
}

Screen::Screen()
{
	copy(&Viewport);
	fill(0);
}

Screen::Screen(int color)
{
	copy(&Viewport);
	fill(color);
}

void Screen::drawShape(char *name, int frame, int x, int y)
{
	if (name) {
		FarBuffer shape(name);
		DrawFrame(&Viewport, x, y, (long) shape.get(), frame, 0);
	}
}

void Screen::drawShape(int entry, char *flexName, int frame, int x, int y)
{
	if (flexName) {
		FarBuffer shape(flexName, entry);
		DrawFrame(&Viewport, x, y, (long) shape.get(), frame, 0);
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
void Screen::paint(int color, unsigned char noCopy)
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
		fill(color);
	node = 0;
	while (List_stepForward(&controls, &node))
		if (((ControlNode *) node)->get()->isVisible()) {
			child = ((ControlNode *) node)->get();
			child->draw();
		}
	if (!noCopy) {
		unsigned char cursor;

		if ((cursor = IsCursorDrawn()) != 0) {
			DisableCursorTracking();
			DrawCursorInto(&Viewport);
		}
		ScreenView.copyFrom(&Viewport);
		if (cursor) {
			EraseCursorFrom(&Viewport);
			EnableCursorTracking();
		}
	}
}

BlendSprite::BlendSprite(void far *data, char back, Screen *screen, char keep) : Sprite(data, back, screen, keep)
{
	table = 0;
	highlightTable = 0;
}

void BlendSprite::setTable(void far *t)
{
	table = t;
}

void BlendSprite::setHighlightTable(void far *t)
{
	highlightTable = t;
}

void BlendSprite::draw()
{
	void far *blend = table;

	if (highlighted)
		blend = highlightTable;
	if (backFrame == 1)
		DrawFrameTranslucent(this, x, y, (long) shape.get(), 0, (long) blend, drawFlags);
	DrawFrameTranslucent(this, x, y, (long) shape.get(), frame, (long) blend, drawFlags);
}

Button::Button(char *flexName, int entry, char back, Screen *screen, char keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	align = ALIGN_LEFT;
}

void Button::draw()
{
	int left = x;
	int top = y;

	switch (align) {
	case ALIGN_CENTRE:
		left -= width >> 1;
		break;
	case ALIGN_RIGHT:
		left -= width;
		break;
	}
	if (backFrame == 1)
		DrawFrame(this, left, top, (long) shape.get(), 0, drawFlags);
	int shown = highlighted ? 1 : 0;
	DrawFrame(this, left, top, (long) shape.get(), shown, drawFlags);
}

unsigned char Button::contains(int px, int py)
{
	int left = x;
	int top = y;

	switch (align) {
	case ALIGN_CENTRE:
		left -= width >> 1;
		break;
	case ALIGN_RIGHT:
		left -= width;
		break;
	}
	Rect bounds;
	GetFrameBounds(&bounds, left, top, (long) shape.get(), frame, drawFlags);
	return bounds.contains(px, py);
}

MovingSprite::MovingSprite(char *name, char back, Screen *screen, char keep) : Sprite(name, back, screen, keep)
{
	startX = startY = endX = endY = steps = 0;
}

MovingSprite::MovingSprite(char *flexName, int entry, char back, Screen *screen, char keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	startX = startY = endX = endY = steps = 0;
}

void MovingSprite::setPath(int x0, int y0, int x1, int y1, int count)
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

void MovingSprite::slideBy(int dx, int dy, int count)
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

BouncingSprite::BouncingSprite(char *name, char back, Screen *screen, char keep)
	: MovingSprite(name, back, screen, keep)
{
}

BouncingSprite::BouncingSprite(char *flexName, int entry, char back, Screen *screen, char keep)
	: MovingSprite(flexName, entry, back, screen, keep)
{
}

void BouncingSprite::setSpeed(int dx, int dy)
{
	speedX = dx;
	speedY = dy;
}

/* Drifts at random; speeds are in thirds of a pixel. */
void BouncingSprite::wander()
{
	int dx = random(7) - 3;
	int dy = random(7) - 3;

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
void BouncingSprite::cycleFrames(int first, int last)
{
	if (frame == last)
		frameStep = -1;
	else if (frame == first)
		frameStep = 1;
	frame += frameStep;
}
