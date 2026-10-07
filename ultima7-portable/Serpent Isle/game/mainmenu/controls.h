#ifndef CONTROLS_H
#define CONTROLS_H

#include "colbuf.h"
#include "view.h"
#include "../shared/farbuf.h"

namespace MainMenu {

using Shared::FarBuffer;

struct Control;

void NullControl(void);

/* A control in a screen's list. */
struct ControlNode : DoubleLink {
	Control *child;
	ControlNode() { child = 0; }
	ControlNode(Control *c) { child = c; }
	Control *get() { return child; }
	void *operator new(size_t);
};

/* A screen's controls, drawn first to last. */
struct ControlList : DoubleList {
	ControlList() : DoubleList() {}
	~ControlList() { List_removeAndDestroyAll(this); }
	void append(Control *child);
	void prepend(Control *child);
};

/* A copy of the view that controls draw on, and the controls on it. */
struct Screen : View {
	ControlList controls;
	int16_t unused;
	Screen();
	Screen(int16_t color);
	~Screen() { clear(); }
	void drawShape(char *name, int16_t frame, int16_t x, int16_t y);
	void drawShape(int16_t entry, char *flexName, int16_t frame, int16_t x, int16_t y);
	void add(Control *child);
	void append(Control *child);
	void drop(Control *child);
	void remove(Control *child);
	void clear();
	virtual void paint(int16_t color, uint8_t noCopy);
};

/* Something drawn on a screen, through its own copy of the screen's view. */
struct Control : View {
	Screen *parent;
	int16_t drawn;                  /* what lies under it is saved */
	int16_t visible;
	uint8_t highlighted;
	int16_t id;
	Control();
	Control(Screen *screen);
	void leave();
	void setParent(Screen *screen);
	void detach();
	void attach(Screen *screen);
	void setHighlight(uint8_t on);
	virtual void saveUnder() = 0;
	virtual void draw() = 0;
	virtual void restoreUnder() = 0;
	virtual uint8_t contains(int16_t x, int16_t y) = 0;
	virtual uint8_t handleKey(int16_t key);
	virtual uint8_t press(int16_t x, int16_t y);
	virtual uint8_t release(int16_t x, int16_t y);
	int16_t isDrawn();
	int16_t isVisible();
	void show();
	void hide();
	int16_t getId();
	void setId(int16_t n);
};

/* One frame of a shape at (x, y); with backFrame set, frame 0 is drawn beneath the current one. */
struct Sprite : Control {
	FarBuffer shape;
	FarBuffer under;
	int8_t keepUnder;
	int8_t backFrame;
	int16_t width, height;
	int16_t frameCount;
	int16_t underX, underY, underFrame;
	int16_t drawFlags;
	int16_t x, y;
	int16_t frame;
	int16_t scale;                  /* 256 is full size */
	int16_t angle;                  /* degrees */
	Sprite();
	Sprite(char *name, int8_t back, Screen *screen, int8_t keep);
	Sprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	Sprite(void *data, int8_t back, Screen *screen, int8_t keep);
	virtual ~Sprite();
	virtual void load(char *name, Screen *screen, int8_t keep, int8_t back);
	virtual void load(char *flexName, int16_t entry, Screen *screen, int8_t keep, int8_t back);
	void init(void *data, Screen *screen, int8_t keep, int8_t back);
	void saveUnder();
	void draw();
	void restoreUnder();
	uint8_t contains(int16_t px, int16_t py);
	void nextFrame();
	uint8_t stepForward();
	uint8_t stepBackward();
	void setFrame(int16_t n);
	void moveTo(int16_t nx, int16_t ny);
	void moveBy(int16_t dx, int16_t dy);
	void setScale(int16_t n);
	void addScale(int16_t n);
	void setAngle(int16_t n);
	void turn(int16_t degrees);
	int16_t getScale();
	int16_t lastFrame();
	void *getData();
};

/* A straight line walked in a number of steps; the error terms carry the remainders. */
struct Path {
	int16_t startX, startY;
	int16_t endX, endY;
	int16_t errorX, errorY;
	int16_t steps;
	int16_t step;
};

/* A sprite that walks a path from start to end. */
struct MovingSprite : Sprite, Path {
	MovingSprite(char *name, int8_t back, Screen *screen, int8_t keep);
	MovingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	void setPath(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t count);
	void slideBy(int16_t dx, int16_t dy, int16_t count);
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
	BouncingSprite(char *name, int8_t back, Screen *screen, int8_t keep);
	BouncingSprite(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	void setSpeed(int16_t dx, int16_t dy);
	void wander();
	void cycleFrames(int16_t first, int16_t last);
};

/* The translucency table a sprite is drawn through. */
struct Blend {
	void *table;
};

/* A sprite drawn through a blend table, or through a second one while highlighted. */
struct BlendSprite : Sprite, Blend {
	void *highlightTable;
	BlendSprite(void *data, int8_t back, Screen *screen, int8_t keep);
	void setTable(void *t);
	void setHighlightTable(void *t);
	void draw();
};

/* How a button stands on its x. */
#define ALIGN_LEFT      0
#define ALIGN_CENTRE    1
#define ALIGN_RIGHT     2

struct Alignment {
	uint8_t align;
};

/* A menu button: a sprite aligned on its x, drawn with frame 1 while highlighted. */
struct Button : Sprite, Alignment {
	Button(char *flexName, int16_t entry, int8_t back, Screen *screen, int8_t keep);
	void setAlign(uint8_t value) { align = value; }
	void draw();
	uint8_t contains(int16_t px, int16_t py);
};

}

#endif
