/* Serpent Isle SI.EXE, overlay segment 325 (file offsets 0x091f50 to 0x092cdb, 3467 bytes).
 * Borland C++ 2.0 -mm -O -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "objref.h"
#include "u7manage.h"
#include "colbuf.h"
#include "u7event.h"
#include "gumps.h"
#include "oops.h"
#include "bltshape.h"
#include "itable.h"
#include "itemcmd.h"
#include "mouse.h"
#include "camera.h"
#include "systimer.h"

void *operator new(size_t);
void operator delete(void *);

struct View;
struct Control;

struct String {
	char *str;
	int16_t len;
	String() { str = 0; len = 0; }
	~String();
	String &operator=(char *);
};

struct Caption : Control {
	int16_t x, y;
	String message;
	Timer delay;
	Caption();
	Caption(int16_t, int16_t, char *);
	void draw(View *);
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
};

struct Rect : Point {
	int16_t x1, y1;
	Rect(int16_t a, int16_t b, int16_t c, int16_t d) : Point(a, b) { x1 = c; y1 = d; }
	int16_t contains(int16_t px, int16_t py) { return (px >= x) & (px <= x1) & (py >= y) & (py <= y1); }
};

extern View Viewport;

ControlNode::ControlNode() { child = 0; }

ControlNode::~ControlNode() {}

ControlNode::ControlNode(Control *value) { child = value; }

Control *ControlNode::get() { return child; }

void *ControlNode::operator new(size_t)
{
	ControlNode *node = ::new ControlNode;
	if (!node) ReportOutOfNearMemory();
	return node;
}

ControlList::ControlList() { savedHead = 0; savedTail = 0; }

ControlList::~ControlList() { List_removeAndDestroyAll(this); }

void ControlList::save()
{
	savedHead = head; head = 0;
	savedTail = tail; tail = 0;
}

void ControlList::restore()
{
	head = savedHead; savedHead = 0;
	tail = savedTail; savedTail = 0;
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

void ControlList::destroy(Control *child)
{
	DoubleLink *node = 0;
	while (List_stepForward(this, &node)) {
		if (((ControlNode *)node)->child == child) {
			List_removeAndDestroy(this, node);
			break;
		}
	}
}

void ControlList::bringForward(Control *child)
{
	DoubleLink *node = 0;
	while (List_stepForward(this, &node)) {
		if (((ControlNode *)node)->child == child) {
			List_bringToFront(this, node);
			break;
		}
	}
}

Control::Control() { visible = 0; locked = 0; }

void Control::save() { children.save(); }

void Control::restore() { children.restore(); }

uint8_t Control::isVisible() { return visible; }

uint8_t Control::isLocked() { return locked; }

void Control::add(Control *child) { children.append(child); }

void Control::remove(Control *child)
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node)) {
		if (((ControlNode *)node)->get() == child) {
			List_unlink(&children, node);
			break;
		}
	}
}

void Control::lower(Control *child)
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node)) {
		if (((ControlNode *)node)->get() == child) {
			List_unlink(&children, node);
			children.prepend(child);
			break;
		}
	}
}

void Control::paint(View *target)
{
	DoubleLink *node = 0;
	Control *child;
	draw(target);
	while (List_stepBackward(&children, &node)) {
		child = ((ControlNode *)node)->get();
		child->paint(target);
	}
}

void Control::show()
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node))
		((ControlNode *)node)->get()->show();
	if (!locked) visible = 1;
}

void Control::hide()
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node))
		((ControlNode *)node)->get()->hide();
	if (!locked) visible = 0;
}

void Control::lock()
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node))
		((ControlNode *)node)->get()->lock();
	locked = 1;
}

void Control::unlock()
{
	DoubleLink *node = 0;
	while (List_stepForward(&children, &node))
		((ControlNode *)node)->get()->unlock();
	locked = 0;
}

Draggable::~Draggable() { release(); }

void Draggable::allocate(int16_t size) { saved = gShapeManager.allocateBlock(size, 0x7fff, 0); }

void Draggable::release()
{
	if (saved != -1) gShapeManager.releaseBlock(saved);
	saved = -1;
}

uint8_t Sprite::handle(MouseState *) { return 0; }

void Sprite::moveTo(int16_t a, int16_t b) { x = a; y = b; }

int16_t Sprite::getFrame() { return frame; }

void Sprite::setFrame(int16_t n) { frame = n < frameCount ? n : 0; }

int16_t Sprite::getShape() { return shape; }

Sprite::Sprite()
{
	shape = 1423;
	frame = 0;
	frameCount = ShapeManager_getFrameCount(&gShapeManager, shape);
}

Sprite::Sprite(int16_t n)
{
	shape = n + 1423;
	frame = 0;
	frameCount = ShapeManager_getFrameCount(&gShapeManager, shape);
}

void Sprite::setShape(int16_t n)
{
	shape = n;
	frame = 0;
	frameCount = ShapeManager_getFrameCount(&gShapeManager, n);
}

void Sprite::draw(View *target)
{
	if (visible)
		ShapeManager_draw(&gShapeManager, target, x, y, shape, frame, 0, 0);
}

uint8_t ImageButton::handle(MouseState *state)
{
	if (!visible)
		return 0;
	uint8_t unusedInside = 0;
	int16_t mx = MouseState_getX(state);
	int16_t my = state->y;
	if (!gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mx, my)))
		return 0;
	if (state->action == MOUSE_DOUBLE_CLICK)
		return BUTTON_DOUBLE_CLICK;
	if (state->action == MOUSE_CLICK) {
		while (!GameInput.isButtonReleased(1))
			plat_yield();
		if (!gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mx, my)))
			return 0;
		PlaySoundSimple(134);
		return BUTTON_CLICKED;
	}
	return 0;
}

void SizedSprite::size(int16_t *w, int16_t *h) { *w = width; *h = height; }

SizedSprite::SizedSprite()
{
	gShapeManager.getShapeSize(&width, &height, shape);
	x = 0;
	y = 0;
	frame = 0;
}

SizedSprite::SizedSprite(int16_t n) : Sprite(n)
{
	gShapeManager.getShapeSize(&width, &height, shape);
	x = 0;
	y = 0;
	frame = 0;
}

void SizedSprite::press()
{
	frame = 1;
	draw(&Viewport);
	CopyFrameBuffer();
}

void SizedSprite::release()
{
	frame = 0;
	draw(&Viewport);
	CopyFrameBuffer();
}

uint8_t SizedSprite::handle(MouseState *state)
{
	if (!visible)
		return 0;
	uint8_t inside = 0;
	int16_t mx, my;
	Rect bounds(x - width, y - height, x, y);
	mx = MouseState_getX(state);
	my = state->y;
	if (!bounds.contains(mx, my))
		return 0;
	if (!gShapeManager.isCursorInBounds(shape, frame, (Point &)bounds.x1, Point(mx, my)))
		return 0;
	if (state->action == MOUSE_CLICK) {
		inside = 1;
		press();
		PlaySoundSimple(134);
		{
			Timer pause(INT32_C(16));
			Timer_restart(&pause);
			while (!Timer_hasFinished(&pause))
				plat_yield();
		}
		do {
			plat_yield();
			mx = MouseState_getX(GetLastMouseState());
			my = GetLastMouseState()->y;
			inside = gShapeManager.isCursorInBounds(shape, frame, (Point &)bounds.x1, Point(mx, my));
			if (!inside && frame) {
				release();
				PlaySoundSimple(134);
			} else if (inside && !frame) {
				press();
				PlaySoundSimple(134);
			}
		} while (!GameInput.isButtonReleased(1));
		int16_t result;
		if (frame)
			result = BUTTON_CLICKED;
		else
			result = 12;
		release();
		return result;
	}
	return 0;
}

uint8_t CounterSprite::handle(MouseState *state)
{
	if (!visible)
		return 0;
	int16_t mx = MouseState_getX(state);
	int16_t my = state->y;
	if (!gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mx, my)))
		return 0;
	if (state->action == MOUSE_CLICK) {
		advance();
		while (!GameInput.isButtonReleased(1))
			plat_yield();
		PlaySoundSimple(134);
		return frame ? BUTTON_ON : BUTTON_OFF;
	}
	return 0;
}

void CounterSprite::advance() { frame = frame != 0 ? 0 : 1; }

SpecialSprite::SpecialSprite() { frame = 0; }

SpecialSprite::SpecialSprite(int16_t n) : SizedSprite(n) { frame = 0; }

uint8_t SpecialSprite::handle(MouseState *state)
{
	if (!visible)
		return 0;
	int16_t mx = MouseState_getX(state);
	int16_t my = state->y;
	if (!gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mx, my)))
		return 0;
	if (state->action == MOUSE_CLICK) {
		advance();
		while (!GameInput.isButtonReleased(1))
			plat_yield();
		mx = GetMouseX();
		my = GetMouseY();
		PlaySoundSimple(134);
		if (gShapeManager.isCursorInBounds(shape, frame, Point(x, y), Point(mx, my)))
			return BUTTON_ON;
		return BUTTON_OFF;
	}
	return 0;
}

void SpecialSprite::advance() { if (++frame == frameCount) frame = 0; }

Caption::Caption() { x = -1; }

Caption::Caption(int16_t a, int16_t b, char *s)
{
	x = a;
	y = b;
	message = s;
	Timer_set(&delay, INT32_C(120));
}

void Caption::draw(View *target)
{
	View *old;
	if (!Timer_hasFinished(&delay)) {
		old = YellowTextPrinter.target;
		YellowTextPrinter.target = target;
		YellowTextPrinter.setFont(0);
		YellowTextPrinter.x = x;
		YellowTextPrinter.y = y;
		YellowTextPrinter.printString(message.str);
		YellowTextPrinter.target = old;
	}
}

void Caption::moveTo(int16_t, int16_t) {}

uint8_t Caption::handle(MouseState *) { return 0; }
