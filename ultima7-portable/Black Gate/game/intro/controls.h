#ifndef INTRO_CONTROLS_H
#define INTRO_CONTROLS_H

#include "view.h"
#include "colbuf.h"
#include "../shared/farbuf.h"

namespace Intro {

struct Sprite;

/* A sprite in a screen's list. */
struct ControlNode : DoubleLink {
	Sprite *child;
	ControlNode() { child = 0; }
	ControlNode(Sprite *c) { child = c; }
};

/* A screen's sprites, drawn first to last. */
struct ControlList : DoubleList {
	ControlList() : DoubleList() {}
	~ControlList() { List_removeAndDestroyAll(this); }
	void append(Sprite *child);
	void prepend(Sprite *child);
};

/* A cleared copy of the view that sprites draw on, and the sprites on it. */
struct Screen : View {
	ControlList controls;
	uint32_t paintTime;         /* how long a paint took the original machine, in microseconds */
	Screen();
	virtual ~Screen() { clear(); }
	virtual void drawShape(char *name, int16_t frame, int16_t x, int16_t y);
	virtual void drawShape(int16_t entry, char *flexName, int16_t frame, int16_t x, int16_t y);
	void add(Sprite *child);
	void append(Sprite *child);
	void drop(Sprite *child);
	void remove(Sprite *child);
	void clear();
	void paint(int16_t color, uint8_t noCopy);
};

/* One frame of a shape at (x, y), drawn through its own copy of the screen's view. */
struct Sprite : View {
	Screen *parent;
	Shared::FarBuffer shape;
	Shared::FarBuffer under;
	int8_t keepUnder;
	int8_t backFrame;           /* frame 0 is drawn beneath the current one */
	int16_t width, height;
	int16_t drawn;              /* what lies under it is saved */
	int16_t frameCount;
	int16_t visible;
	int16_t underX, underY, underFrame;
	int16_t x, y;
	int16_t frame;
	int16_t scale;              /* 256 is full size */
	int16_t angle;              /* degrees */
	Sprite()
	{
		parent = 0;
		keepUnder = 0;
		drawn = 0;
		x = 0;
		y = 0;
		scale = 256;
		backFrame = 0;
		visible = 1;
	}
	Sprite(char *name, int8_t back, Screen *screen, int8_t keep) { load(name, screen, keep, back); }
	Sprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
	{
		load(flexName, entry, screen, keep, back);
	}
	virtual ~Sprite();
	virtual void load(char *name, Screen *screen, int8_t keep, int8_t back);
	virtual void load(char *flexName, int16_t entry, Screen *screen, int8_t keep, int8_t back);
	virtual void saveUnder();
	virtual void draw();
	virtual void restoreUnder();
	void leave();
	void detach();
	void attach(Screen *screen);
	void setParent(Screen *screen);
	void nextFrame();
	void setFrame(int16_t n) { frame = n; }
	void moveTo(int16_t nx, int16_t ny) { x = nx; y = ny; }
};

/* A straight line walked in a number of steps, scaling and turning on the way; the error terms
 * carry the remainders. */
struct Path {
	int16_t startX, startY;
	int16_t endX, endY;
	int16_t startScale, endScale;
	int16_t startAngle, endAngle;
	int16_t errorX, errorY;
	int16_t errorScale, errorAngle;
	int16_t steps;
	int16_t step;
};

/* A sprite that walks a path from start to end. */
struct MovingSprite : Sprite, Path {
	MovingSprite() {}
	MovingSprite(char *name, int8_t back, Screen *screen, int8_t keep);
	MovingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	void setPath(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t scale0, int16_t scale1,
		int16_t angle0, int16_t angle1, int16_t count);
	void slideBy(int16_t dx, int16_t dy, int16_t dscale, int16_t dangle, int16_t count);
	void advance();
	void finish();
};

/* A speed in thirds of a pixel, and the direction frames are stepped in. */
struct Drift {
	int16_t speedX, speedY;
	int16_t frameStep;
};

/* A moving sprite that drifts at random, turns back at the screen's edges and cycles its frames. */
struct BouncingSprite : MovingSprite, Drift {
	BouncingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep)
		: MovingSprite(flexName, entry, back, screen, keep) {}
	void wander();
	void cycleFrames(int16_t first, int16_t last);
};

/* The Guardian's head, with eyes and a mouth drawn over it; the mouth's frame is picked from a
 * mood (its row) and a shape (its column). */
struct Guardian : Sprite {
	Sprite eyes;
	Sprite mouth;
	uint8_t mood;
	uint8_t mouthShape;         /* 3 closes the mouth */
	Guardian(Screen *screen);
	void moveTo(int16_t nx, int16_t ny);
	void draw();
	void restoreUnder();
	void setMood(uint8_t m);
	void setMouthShape(uint8_t s);
};

/* Shape drawing for shapes held in far buffers. */
void DrawShape(View *view, int16_t x, int16_t y, Shared::FarBuffer *shape, int16_t frame);

}

#endif
