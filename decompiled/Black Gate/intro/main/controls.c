/* Black Gate INTRO.EXE, resident segment 5 (file offsets 0x00988d to 0x00a707, 3706 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -d rebuilds it byte for byte as C++.
 */

#include <stdlib.h>
#include "oops.h"
#include "controls.h"

#define ENDSHAPE_HEAD   30
#define ENDSHAPE_EYES   31
#define ENDSHAPE_MOUTH  32
#define MOUTH_CLOSED    10      /* frame of the shut mouth */
#define SIGN(n)         ((n) < 0 ? -1 : 1)

extern char *StaticPath;
char *DataPath(char *dir, char *name);

void *ControlNode::operator new(unsigned)
{
	ControlNode *node = ::new ControlNode;

	if (!node)
		ReportOutOfNearMemory();
	return node;
}

void ControlList::append(Sprite *child)
{
	ControlNode *node = new ControlNode(child);

	node->child = child;
	List_insertAtTail(this, node);
}

void ControlList::prepend(Sprite *child)
{
	ControlNode *node = new ControlNode(child);

	node->child = child;
	List_insertAtHead(this, node);
}

void Screen::drawShape(char *name, int frame, int x, int y)
{
	if (name) {
		FarBuffer shape;

		shape.load(name);
		DrawEmsFrame(&Viewport, x, y, shape.data, frame);
	}
}

void Screen::drawShape(int entry, char *flexName, int frame, int x, int y)
{
	if (flexName) {
		FarBuffer shape;

		shape.load(flexName, entry);
		DrawEmsFrame(&Viewport, x, y, shape.data, frame);
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
void Screen::paint(int color, unsigned char noCopy)
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
		FillView(this, color);
	node = 0;
	while (List_stepForward(&controls, &node))
		if (((ControlNode *) node)->child->visible)
			((ControlNode *) node)->child->draw();
	if (!noCopy)
		CopyView(&Viewport, &ScreenView);
}

Sprite::~Sprite()
{
	detach();
}

void Sprite::load(char *name, Screen *screen, char keep, char back)
{
	keepUnder = keep;
	shape.load(name);
	width = GetEmsMaxFrameWidth(shape.data);
	height = GetEmsMaxFrameHeight(shape.data);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	frameCount = GetEmsShapeFrameCount(shape.data);
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
	shape.load(flexName, entry);
	width = GetEmsMaxFrameWidth(shape.data);
	height = GetEmsMaxFrameHeight(shape.data);
	if (keepUnder)
		under.allocate(width * height);
	visible = 1;
	frameCount = GetEmsShapeFrameCount(shape.data);
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
		SaveUnderEmsFrame(this, under.data, x, y, shape.data, frame);
		underFrame = frame;
		underX = x;
		underY = y;
		drawn = 1;
	}
}

void Sprite::draw()
{
	if (backFrame == 1)
		DrawScaledFrame(this, x, y, shape.data, 0, angle, scale, 0);
	DrawScaledFrame(this, x, y, shape.data, frame, angle, scale, 0);
}

void Sprite::restoreUnder()
{
	if (keepUnder)
		RestoreUnderEmsFrame(this, under.data, underX, underY, shape.data, underFrame);
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

MovingSprite::MovingSprite(char *name, char back, Screen *screen, char keep) : Sprite(name, back, screen, keep)
{
	startX = startY = endX = endY = startScale = endScale = steps = 0;
}

MovingSprite::MovingSprite(char *flexName, int entry, char back, Screen *screen, char keep)
	: Sprite(flexName, entry, back, screen, keep)
{
	startX = startY = endX = endY = startScale = endScale = steps = 0;
}

void MovingSprite::setPath(int x0, int y0, int x1, int y1, int scale0, int scale1, int angle0, int angle1,
	int count)
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

void MovingSprite::slideBy(int dx, int dy, int dscale, int dangle, int count)
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
void Guardian::moveTo(int nx, int ny)
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

void Guardian::setMood(unsigned char m)
{
	mood = m;
	mouth.frame = mood * 3 + mouthShape + 1;
}

void Guardian::setMouthShape(unsigned char s)
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
	x += (speedX += dx) / 3;
	y += (speedY += dy) / 3;
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
