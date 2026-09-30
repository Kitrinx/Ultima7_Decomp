#ifndef CONTROLS_H
#define CONTROLS_H

#include "colbuf.h"
#include "view.h"
#include "farbuf.h"

struct Control;

void NullControl(void);

/* A control in a screen's list. */
struct ControlNode : DoubleLink {
	Control *child;
	ControlNode() { child = 0; }
	ControlNode(Control *c) { child = c; }
	void *operator new(unsigned);
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
	int unused;
	Screen();
	Screen(int color);
	~Screen() { clear(); }
	void drawShape(char *name, int frame, int x, int y);
	void drawShape(int entry, char *flexName, int frame, int x, int y);
	void add(Control *child);
	void append(Control *child);
	void drop(Control *child);
	void remove(Control *child);
	void clear();
	virtual void paint(int color, unsigned char noCopy);
};

/* Something drawn on a screen, through its own copy of the screen's view. */
struct Control : View {
	Screen *parent;
	int drawn;                  /* what lies under it is saved */
	int visible;
	unsigned char highlighted;
	int id;
	Control();
	Control(Screen *screen);
	void leave();
	void setParent(Screen *screen);
	void detach();
	void attach(Screen *screen);
	void setHighlight(unsigned char on);
	virtual void saveUnder() = 0;
	virtual void draw() = 0;
	virtual void restoreUnder() = 0;
	virtual unsigned char contains(int x, int y) = 0;
	virtual unsigned char handleKey(int key);
	virtual unsigned char press(int x, int y);
	virtual unsigned char release(int x, int y);
	int isDrawn();
	int isVisible();
	void show();
	void hide();
	int getId();
	void setId(int n);
};

/* One frame of a shape at (x, y); with backFrame set, frame 0 is drawn beneath the current one. */
struct Sprite : Control {
	FarBuffer shape;
	FarBuffer under;
	char keepUnder;
	char backFrame;
	int width, height;
	int frameCount;
	int underX, underY, underFrame;
	int drawFlags;
	int x, y;
	int frame;
	int scale;                  /* 256 is full size */
	int angle;                  /* degrees */
	Sprite();
	Sprite(char *name, char back, Screen *screen, char keep);
	Sprite(char *flexName, int entry, char back, Screen *screen, char keep);
	Sprite(void far *data, char back, Screen *screen, char keep);
	virtual ~Sprite();
	virtual void load(char *name, Screen *screen, char keep, char back);
	virtual void load(char *flexName, int entry, Screen *screen, char keep, char back);
	void init(void far *data, Screen *screen, char keep, char back);
	void saveUnder();
	void draw();
	void restoreUnder();
	unsigned char contains(int px, int py);
	void nextFrame();
	unsigned char stepForward();
	unsigned char stepBackward();
	void setFrame(int n);
	void moveTo(int nx, int ny);
	void moveBy(int dx, int dy);
	void setScale(int n);
	void addScale(int n);
	void setAngle(int n);
	void turn(int degrees);
	int getScale();
	int lastFrame();
	void far *getData();
};

/* A straight line walked in a number of steps; the error terms carry the remainders. */
struct Path {
	int startX, startY;
	int endX, endY;
	int errorX, errorY;
	int steps;
	int step;
};

/* A sprite that walks a path from start to end. */
struct MovingSprite : Sprite, Path {
	MovingSprite(char *name, char back, Screen *screen, char keep);
	MovingSprite(char *flexName, int entry, char back, Screen *screen, char keep);
	void setPath(int x0, int y0, int x1, int y1, int count);
	void slideBy(int dx, int dy, int count);
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
	BouncingSprite(char *name, char back, Screen *screen, char keep);
	BouncingSprite(char *flexName, int entry, char back, Screen *screen, char keep);
	void setSpeed(int dx, int dy);
	void wander();
	void cycleFrames(int first, int last);
};

/* The translucency table a sprite is drawn through. */
struct Blend {
	void far *table;
};

/* A sprite drawn through a blend table, or through a second one while highlighted. */
struct BlendSprite : Sprite, Blend {
	void far *highlightTable;
	BlendSprite(void far *data, char back, Screen *screen, char keep);
	void setTable(void far *t);
	void setHighlightTable(void far *t);
	void draw();
};

/* How a button stands on its x. */
#define ALIGN_LEFT      0
#define ALIGN_CENTRE    1
#define ALIGN_RIGHT     2

struct Alignment {
	unsigned char align;
};

/* A menu button: a sprite aligned on its x, drawn with frame 1 while highlighted. */
struct Button : Sprite, Alignment {
	Button(char *flexName, int entry, char back, Screen *screen, char keep);
	void draw();
	unsigned char contains(int px, int py);
};

#endif
