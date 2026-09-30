#ifndef CONTROLS_H
#define CONTROLS_H

#include "colbuf.h"
#include "view.h"
#include "farbuf.h"
#include "lowlevel.h"

struct Sprite;

/* A sprite in a screen's list. */
struct ControlNode : DoubleLink {
	Sprite *child;
	ControlNode() { child = 0; }
	ControlNode(Sprite *c) { child = c; }
	void *operator new(unsigned);
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
	Screen() { *(View *) this = Viewport; FillView(this, 0); }
	~Screen() { clear(); }
	virtual void drawShape(char *name, int frame, int x, int y);
	virtual void drawShape(int entry, char *flexName, int frame, int x, int y);
	void add(Sprite *child);
	void append(Sprite *child);
	void drop(Sprite *child);
	void remove(Sprite *child);
	void clear();
	void paint(int color, unsigned char noCopy);
};

/* One frame of a shape at (x, y), drawn through its own copy of the screen's view. */
struct Sprite : View {
	Screen *parent;
	FarBuffer shape;
	FarBuffer under;
	char keepUnder;
	char backFrame;             /* frame 0 is drawn beneath the current one */
	int width, height;
	int drawn;                  /* what lies under it is saved */
	int frameCount;
	int visible;
	int underX, underY, underFrame;
	int x, y;
	int frame;
	int scale;                  /* 256 is full size */
	int angle;                  /* degrees */
	Sprite()
	{
		keepUnder = 0;
		drawn = 0;
		x = 0;
		y = 0;
		scale = 256;
		backFrame = 0;
		visible = 1;
	}
	Sprite(char *name, char back, Screen *screen, char keep) { load(name, screen, keep, back); }
	Sprite(char *flexName, int entry, char back, Screen *screen, char keep)
	{
		load(flexName, entry, screen, keep, back);
	}
	virtual ~Sprite();
	virtual void load(char *name, Screen *screen, char keep, char back);
	virtual void load(char *flexName, int entry, Screen *screen, char keep, char back);
	virtual void saveUnder();
	virtual void draw();
	virtual void restoreUnder();
	void leave();
	void detach();
	void attach(Screen *screen);
	void setParent(Screen *screen);
	void nextFrame();
	void setFrame(int n) { frame = n; }
	void moveTo(int nx, int ny) { x = nx; y = ny; }
};

/* A straight line walked in a number of steps, scaling and turning on the way; the error terms
 * carry the remainders. */
struct Path {
	int startX, startY;
	int endX, endY;
	int startScale, endScale;
	int startAngle, endAngle;
	int errorX, errorY;
	int errorScale, errorAngle;
	int steps;
	int step;
};

/* A sprite that walks a path from start to end. */
struct MovingSprite : Sprite, Path {
	MovingSprite() {}
	MovingSprite(char *name, char back, Screen *screen, char keep);
	MovingSprite(char *flexName, int entry, char back, Screen *screen, char keep);
	void setPath(int x0, int y0, int x1, int y1, int scale0, int scale1, int angle0, int angle1,
		int count);
	void slideBy(int dx, int dy, int dscale, int dangle, int count);
	void advance();
	void finish();
};

/* A speed in thirds of a pixel, and the direction frames are stepped in. */
struct Drift {
	int speedX, speedY;
	int frameStep;
};

/* A moving sprite that drifts at random, turns back at the screen's edges and cycles its frames. */
struct BouncingSprite : MovingSprite, Drift {
	BouncingSprite(char *flexName, int entry, char back, Screen *screen, char keep)
		: MovingSprite(flexName, entry, back, screen, keep) {}
	void wander();
	void cycleFrames(int first, int last);
};

/* The Guardian's head, with eyes and a mouth drawn over it; the mouth's frame is picked from a
 * mood (its row) and a shape (its column). */
struct Guardian : Sprite {
	Sprite eyes;
	Sprite mouth;
	unsigned char mood;
	unsigned char mouthShape;   /* 3 closes the mouth */
	Guardian(Screen *screen);
	void moveTo(int nx, int ny);
	void draw();
	void restoreUnder();
	void setMood(unsigned char m);
	void setMouthShape(unsigned char s);
};

#endif
