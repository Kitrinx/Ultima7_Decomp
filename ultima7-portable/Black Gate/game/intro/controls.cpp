/* Black Gate INTRO.EXE, controls.c: the intro's sprite engine. A Screen is a copy of the draw buffer
 * with a list of sprites; painting puts back what each sprite covered, saves what it will cover,
 * draws it, and copies the buffer to the screen.
 */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"
#include "colbuf.h"
#include "u7manage.h"
#include "controls.h"
#include "scalfram.h"
#include "pacing.h"
#include "intro.h"

namespace Intro {

#define ENDSHAPE_HEAD   30
#define ENDSHAPE_EYES   31
#define ENDSHAPE_MOUTH  32
#define MOUTH_CLOSED    10      /* frame of the shut mouth */
#define SIGN(n)         ((n) < 0 ? -1 : 1)

/* A real-mode far pointer kept only 20 bits of a frame's offset; ENDSHAPE.FLX's monitor has junk
 * in the high bits of its only offset. */
static void MaskFrameOffset(int32_t shape, int16_t frame)
{
	int32_t entry = shape + (frame + 1) * 4;

	if (LinearGet16(shape + 4) > (uint16_t) ((frame + 1) * 4))
		LinearPut32(entry, LinearGet32(entry) & 0xfffff);
}

void DrawShape(View *view, int16_t x, int16_t y, Shared::FarBuffer *shape, int16_t frame)
{
	MaskFrameOffset(shape->linear(), frame);
	DrawFrame(view, x, y, shape->linear(), frame, 0);
}

void ControlList::append(Sprite *child)
{
	List_insertAtTail(this, new ControlNode(child));
}

void ControlList::prepend(Sprite *child)
{
	List_insertAtHead(this, new ControlNode(child));
}

Screen::Screen()
{
	paintTime = PAINT_TIME_LIGHT;
	*(View *) this = Viewport;
	FillView((View *) this, 0);
}

void Screen::drawShape(char *name, int16_t frame, int16_t x, int16_t y)
{
	if (name) {
		Shared::FarBuffer shape;

		shape.load(name);
		DrawShape(&Viewport, x, y, &shape, frame);
	}
}

void Screen::drawShape(int16_t entry, char *flexName, int16_t frame, int16_t x, int16_t y)
{
	if (flexName) {
		Shared::FarBuffer shape;

		shape.load(flexName, entry);
		DrawShape(&Viewport, x, y, &shape, frame);
	}
}

void Screen::add(Sprite *child)
{
	controls.append(child);
	child->setParent(this);
}

void Screen::append(Sprite *child)
{
	controls.append(child);
}

/* Takes a sprite off the screen and puts back what lay under it. */
void Screen::drop(Sprite *child)
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node)) {
		if (((ControlNode *) node)->child == child) {
			List_unlink(&controls, node);
			((ControlNode *) node)->child->leave();
			break;
		}
	}
}

void Screen::remove(Sprite *child)
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node)) {
		if (((ControlNode *) node)->child == child) {
			List_unlink(&controls, node);
			break;
		}
	}
}

void Screen::clear()
{
	DoubleLink *node = 0;

	while (List_stepForward(&controls, &node))
		((ControlNode *) node)->child->leave();
	List_removeAndDestroyAll(&controls);
}

/* Redraws every visible sprite, over a fresh fill of the color or, with -1, over what they saved.
 * Unless told not to, the result is copied to the screen. */
void Screen::paint(int16_t color, uint8_t noCopy)
{
	DoubleLink *node = 0;

	if (color == -1) {
		while (List_stepForward(&controls, &node))
			if (((ControlNode *) node)->child->drawn)
				((ControlNode *) node)->child->restoreUnder();
		node = 0;
		while (List_stepForward(&controls, &node))
			if (((ControlNode *) node)->child->visible)
				((ControlNode *) node)->child->saveUnder();
	} else
		FillView((View *) this, (uint8_t) color);
	node = 0;
	while (List_stepForward(&controls, &node))
		if (((ControlNode *) node)->child->visible)
			((ControlNode *) node)->child->draw();
	if (!noCopy)
		CopyView(&Viewport, &ScreenView);
	SpendTime(paintTime);
}

Sprite::~Sprite()
{
	detach();
}

void Sprite::load(char *name, Screen *screen, int8_t keep, int8_t back)
{
	keepUnder = keep;
	shape.load(name);
	width = GetMaxFrameWidth(shape.linear(), 0);
	height = GetMaxFrameHeight(shape.linear(), 0);
	if (keepUnder)
		under.allocate((int32_t) width * height);
	visible = 1;
	frameCount = GetShapeFrameCount(shape.linear(), 0);
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
	shape.load(flexName, entry);
	width = GetMaxFrameWidth(shape.linear(), 0);
	height = GetMaxFrameHeight(shape.linear(), 0);
	if (keepUnder)
		under.allocate((int32_t) width * height);
	visible = 1;
	frameCount = GetShapeFrameCount(shape.linear(), 0);
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
		SaveUnderFrame(this, under.linear(), x, y, shape.linear(), frame, 0);
		underFrame = frame;
		underX = x;
		underY = y;
		drawn = 1;
	}
}

void Sprite::draw()
{
	if (backFrame == 1)
		DrawScaledFrame(this, x, y, shape.linear(), 0, angle, scale, 0);
	DrawScaledFrame(this, x, y, shape.linear(), frame, angle, scale, 0);
}

void Sprite::restoreUnder()
{
	if (keepUnder)
		RestoreUnderFrame(this, under.linear(), underX, underY, shape.linear(), underFrame, 0);
}

/* Puts back what lay under the sprite; its screen has already let go of it. */
void Sprite::leave()
{
	if (visible && drawn)
		restoreUnder();
	parent = 0;
}

void Sprite::detach()
{
	if (visible && drawn)
		restoreUnder();
	if (parent) {
		parent->remove(this);
		parent = 0;
	}
}

void Sprite::attach(Screen *screen)
{
	if (parent)
		parent->remove(this);
	parent = screen;
	*(View *) this = *screen;
	parent->append(this);
}

void Sprite::setParent(Screen *screen)
{
	if (parent)
		parent->remove(this);
	parent = screen;
	*(View *) this = *screen;
}

void Sprite::nextFrame()
{
	frame++;
	setFrame(frame %= frameCount);
	if (backFrame == 1 && frame == 0)
		frame = 1;
}

MovingSprite::MovingSprite(char *name, int8_t back, Screen *screen, int8_t keep) : Sprite(name, back, screen, keep)
{
	startX = startY = endX = endY = startScale = endScale = steps = 0;
}

MovingSprite::MovingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	startX = startY = endX = endY = startScale = endScale = steps = 0;
}

void MovingSprite::setPath(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t scale0, int16_t scale1,
	int16_t angle0, int16_t angle1, int16_t count)
{
	startX = x0;
	startY = y0;
	endX = x1;
	endY = y1;
	startScale = scale0;
	endScale = scale1;
	startAngle = angle0;
	endAngle = angle1;
	steps = count;
	step = 0;
	errorX = errorY = errorScale = errorAngle = 0;
	x = startX;
	y = startY;
	scale = startScale;
	angle = startAngle;
}

void MovingSprite::slideBy(int16_t dx, int16_t dy, int16_t dscale, int16_t dangle, int16_t count)
{
	setPath(x, y, x + dx, y + dy, scale, scale + dscale, angle, angle + dangle, count);
}

/* One step along the line; the remainders of the division carry over as in Bresenham's. */
void MovingSprite::advance()
{
	if (step <= steps) {
		step++;
		x += (endX - startX) / steps;
		y += (endY - startY) / steps;
		scale += (endScale - startScale) / steps;
		angle += (endAngle - startAngle) / steps;
		errorX += (endX - startX) % steps;
		errorY += (endY - startY) % steps;
		errorScale += (endScale - startScale) % steps;
		errorAngle += (endAngle - startAngle) % steps;
		if (abs(errorX) > steps) {
			x += SIGN(errorX);
			errorX -= SIGN(errorX) * steps;
		}
		if (abs(errorY) > steps) {
			y += SIGN(errorY);
			errorY -= SIGN(errorY) * steps;
		}
		if (abs(errorScale) > steps) {
			scale += SIGN(errorScale);
			errorScale -= SIGN(errorScale) * steps;
		}
		if (abs(errorAngle) > steps) {
			angle += SIGN(errorAngle);
			errorAngle -= SIGN(errorAngle) * steps;
		}
	}
}

void MovingSprite::finish()
{
	x = endX;
	y = endY;
	scale = endScale;
	angle = endAngle;
	step = steps;
}

Guardian::Guardian(Screen *screen)
	: Sprite(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_HEAD, 1, screen, 1)
{
	eyes.load(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_EYES, screen, 1, 0);
	mouth.load(DataPath(StaticPath, "endshape.flx"), ENDSHAPE_MOUTH, screen, 1, 1);
	mouth.frame = 1;
	mouthShape = 0;
	mood = 2;
}

/* The eyes sit 48 rows above the head's origin and the mouth 11. */
void Guardian::moveTo(int16_t nx, int16_t ny)
{
	Sprite::moveTo(nx, ny);
	mouth.moveTo(nx, ny - 11);
	eyes.moveTo(nx, ny - 48);
}

void Guardian::draw()
{
	eyes.draw();
	mouth.draw();
	Sprite::draw();
}

void Guardian::restoreUnder()
{
	eyes.restoreUnder();
	mouth.restoreUnder();
	Sprite::restoreUnder();
}

void Guardian::setMood(uint8_t m)
{
	mood = m;
	mouth.frame = mood * 3 + mouthShape + 1;
}

void Guardian::setMouthShape(uint8_t s)
{
	if (s == 3)
		mouth.frame = MOUTH_CLOSED;
	else {
		mouthShape = s;
		mouth.frame = mood * 3 + mouthShape + 1;
	}
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
	x += (speedX += dx) / 3;
	y += (speedY += dy) / 3;
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
