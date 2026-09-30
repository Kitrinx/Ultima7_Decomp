#ifndef GUMPS_H
#define GUMPS_H

/* Controls, sprites and gumps. */

#include "objref.h"
#include "colbuf.h"

struct View;
struct MouseState;
struct Control;
struct NPCRef;

/* One item shown in a container, with its place. */
struct ItemNode : DoubleLink {
	uint8_t x, y;
	objref object;
	uint8_t active;
};

/* A control in a list of controls. */
struct ControlNode : DoubleLink {
	Control *child;
	ControlNode();
	virtual ~ControlNode();
	ControlNode(Control *);
	Control *get();
	void *operator new(size_t);
};

struct Point {
	int16_t x, y;
	Point() { x = 0; y = 0; }
	Point(int16_t a, int16_t b) { x = a; y = b; }
};

struct Rectangle : Point {
	int16_t x1, y1;
	Rectangle() : Point() { x1 = 0; y1 = 0; }
	void set(int16_t a, int16_t b, int16_t c, int16_t d) { x = a; y = b; x1 = c; y1 = d; }
	int16_t width() { return x1 - x + 1; }
	int16_t height() { return y1 - y + 1; }
	int16_t contains(int16_t a, int16_t b) { return (x <= a) & (x1 >= a) & (y <= b) & (y1 >= b); }
	void moveTo(int16_t a, int16_t b) { x1 = a + (x1 - x); y1 = b + (y1 - y); x = a; y = b; }
};

/* A control's children, with a saved copy of the order. */
struct ControlList : DoubleList {
	DoubleLink *savedHead, *savedTail;
	ControlList();
	virtual ~ControlList();
	void save();
	void restore();
	void append(Control *);
	void prepend(Control *);
	void destroy(Control *);
	void bringForward(Control *);
};

struct Control {
	ControlList children;
	uint8_t visible, locked;
	Control();
	void save();
	void restore();
	uint8_t isVisible();
	uint8_t isLocked();
	void add(Control *);
	void remove(Control *);
	void lower(Control *);
	virtual void paint(View *);
	virtual void moveTo(int16_t, int16_t) = 0;
	virtual uint8_t handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	void show();
	void hide();
	void lock();
	void unlock();
};

struct Sprite : Control {
	int16_t x, y, shape, frame, frameCount;
	Sprite(int16_t);
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
	virtual int16_t getFrame();
	virtual void setFrame(int16_t);
	virtual int16_t getShape();
};

/* A sprite that acts when clicked. */
struct ImageButton : Sprite {
	ImageButton(int16_t n) : Sprite(n) {}
	uint8_t handle(MouseState *);
};

struct SizedSprite : Sprite {
	int16_t width, height;
	SizedSprite(int16_t);
	uint8_t handle(MouseState *);
	virtual void size(int16_t *, int16_t *);
	virtual void press();
	virtual void release();
};

struct CounterSprite : SizedSprite {
	CounterSprite(int16_t n) : SizedSprite(n) { frame = 0; }
	uint8_t handle(MouseState *);
	virtual void advance();
};

struct SpecialSprite : SizedSprite {
	SpecialSprite(int16_t);
	uint8_t handle(MouseState *);
	virtual void advance();
};

/* Saves the screen under a dragged image. */
struct Draggable {
	int16_t saved, image, hotX, hotY, lastX, lastY;
	Draggable() { saved = -1; }
	virtual ~Draggable();
	virtual void pickUp(int16_t, int16_t) = 0;
	virtual void show(int16_t, int16_t) = 0;
	virtual void drop(int16_t, int16_t) = 0;
	virtual void hide() = 0;
	virtual void dragTo(int16_t, int16_t) = 0;
	virtual void drag(MouseState *) = 0;
	void allocate(int16_t);
	void release();
};

/* A control that holds or shows items. */
struct Panel : Control {
	int8_t dirty;
	virtual void moveTo(int16_t, int16_t) = 0;
	virtual uint8_t handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	virtual objref object() = 0;
	virtual uint8_t accepts(objref, int16_t, int16_t) = 0;
	virtual objref selected() = 0;
	virtual int16_t mouseX() = 0;
	virtual int16_t mouseY() = 0;
	virtual int16_t dragX() = 0;
	virtual int16_t dragY() = 0;
	virtual void setDragX(int16_t) = 0;
	virtual void setDragY(int16_t) = 0;
	virtual void refresh(int8_t) = 0;
};

/* The items a container shows, with their places. */
struct ItemList : DoubleList {
	void append(objref, uint8_t, uint8_t);
	void prepend(objref, uint8_t, uint8_t);
	void markStale();
	ItemNode *find(objref);
	void removeStale();
};

struct ItemGrid : Control {
	Rectangle region;
	objref container;
	ItemList items;
	objref selectedItem;
	int16_t clickX, clickY, offsetX, offsetY;
	ItemGrid() {}
	void initialize(objref, uint8_t, uint8_t, uint8_t, uint8_t);
	void refresh();
	void place(objref, uint8_t *, uint8_t *, uint8_t *);
	void build(objref);
	ItemNode *find(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void moveTo(int16_t, int16_t);
	void draw(View *);
	uint8_t accepts(objref, int16_t, int16_t);
	virtual objref object();
};

/* An item dialog: a container, a paperdoll, a spellbook or a stats sheet. */
struct Gump : Draggable, Panel {
	objref displayed;
	int16_t kind, shape;
	SizedSprite closeButton;
	Rectangle bounds;
	uint8_t stats, dragging;
	Gump() : closeButton(2) { stats = 0; dragging = 0; dirty = 0; }
	~Gump();
	void pickUp(int16_t, int16_t);
	void show(int16_t, int16_t);
	void drop(int16_t, int16_t);
	void hide();
	void dragTo(int16_t, int16_t);
	void drag(MouseState *);
	virtual uint8_t handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	virtual void moveTo(int16_t, int16_t) = 0;
	virtual uint8_t accepts(objref, int16_t, int16_t) = 0;
	virtual objref selected() = 0;
	virtual int16_t mouseX() = 0;
	virtual int16_t mouseY() = 0;
	virtual int16_t dragX() = 0;
	virtual int16_t dragY() = 0;
	virtual void setDragX(int16_t) = 0;
	virtual void setDragY(int16_t) = 0;
	virtual void refresh(int8_t) = 0;
	objref object();
	virtual uint8_t findPosition(objref, int16_t *, int16_t *) = 0;
};

struct ContainerGump : Gump {
	ItemGrid contents;
	ContainerGump(objref);
	~ContainerGump();
	void initialize();
	uint8_t handle(MouseState *);
	void draw(View *);
	void moveTo(int16_t, int16_t);
	uint8_t accepts(objref, int16_t, int16_t);
	objref selected();
	int16_t mouseX();
	int16_t mouseY();
	int16_t dragX();
	int16_t dragY();
	void setDragX(int16_t);
	void setDragY(int16_t);
	void refresh(int8_t);
	uint8_t findPosition(objref, int16_t *, int16_t *);
};

struct StatsGump : Gump {
	objref displayedItem;
	int16_t clickX, clickY;
	char unusedField1[65];
	Sprite asleepIcon, poisonedIcon, charmedIcon, hungryIcon, protectedIcon, cursedIcon, paralyzedIcon;
	int16_t dragOffsetX, dragOffsetY;
	StatsGump(objref);
	void initialize();
	void update();
	uint8_t handle(MouseState *);
	void draw(View *);
	void moveTo(int16_t, int16_t);
	uint8_t accepts(objref, int16_t, int16_t);
	objref selected();
	int16_t mouseX();
	int16_t mouseY();
	int16_t dragX();
	int16_t dragY();
	void setDragX(int16_t);
	void setDragY(int16_t);
	void refresh(int8_t);
	uint8_t findPosition(objref, int16_t *, int16_t *);
};

struct ItemSlot : Control {
	int16_t x, y;
	objref carried;
	uint8_t unusedMale;   /* whether the paperdoll is male */
	ItemSlot() {}
	void initialize(objref, uint8_t);
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
	virtual objref object();
	uint8_t accept(objref);
};

struct InventoryGump : Gump {
	ItemSlot slots[12];
	objref selectedItem;
	int16_t clickX, clickY;
	SizedSprite statsButton, diskButton;
	CounterSprite combatButton;
	Sprite twoSlotMark, twoHandedMark;
	SpecialSprite attackModeButton;
	CounterSprite protectButton;
	int16_t dragOffsetX, dragOffsetY;
	InventoryGump(NPCRef);
	~InventoryGump();
	void initialize();
	void update(NPCRef, uint8_t, uint8_t);
	uint8_t findSlot(uint8_t *, objref, int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
	void moveTo(int16_t, int16_t);
	uint8_t accepts(objref, int16_t, int16_t);
	objref selected();
	int16_t mouseX();
	int16_t mouseY();
	int16_t dragX();
	int16_t dragY();
	void setDragX(int16_t);
	void setDragY(int16_t);
	void refresh(int8_t);
	uint8_t findPosition(objref, int16_t *, int16_t *);
};

int16_t MakeShapeFromImage(int16_t);

/* An item carried by the mouse. */
struct ItemDrag : Draggable, Control {
	objref obj;
	int16_t posx, posy;
	ItemDrag(objref, int16_t, int16_t, int16_t, int16_t);
	void pickUp(int16_t, int16_t);
	void show(int16_t, int16_t);
	void drop(int16_t, int16_t);
	void hide();
	void dragTo(int16_t, int16_t);
	void drag(MouseState *);
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
	virtual objref object();
};

/* what a control's handle() asks for; 0 is a click that missed it */
#define GUMP_CLOSE          1
#define GUMP_MOVE           3   /* drag the gump itself */
#define GUMP_OPEN_ITEM      5   /* open the item clicked */
#define GUMP_DRAG_ITEM      6   /* drag the item clicked */
#define GUMP_DROP_HERE      7   /* a dragged item was let go over this gump */
#define GUMP_REDRAW         11
#define GUMP_HANDLED        12  /* the click was used; nothing more to do */
#define GUMP_NO_DROP        13  /* a dragged item was let go where it cannot go */
#define GUMP_SAVE_DIALOG    16
#define GUMP_STATS          17
#define BUTTON_CLICKED      18
#define BUTTON_DOUBLE_CLICK 19
#define GUMP_CAST_SPELL     20
#define BUTTON_ON           21  /* a two-state button turned on */
#define BUTTON_OFF          22
#define GUMP_RESTORED       23  /* a saved game was loaded */
#define GUMP_SELECT_ITEM    24  /* the item clicked is picked, or its name shown */
#define GUMP_USE_ITEM       25
#define GUMP_QUIT           26
#define GUMP_SET_LEADER     27
#define GUMP_CLEAR_LEADER   28
#define GUMP_YES            29
#define GUMP_NO             30
#define GUMP_REFRESH        31  /* combat began: update the inventories */
#define GUMP_PICK_GROUND    32  /* picking, and nothing lies under the mouse */

struct ProportionalTextPrinter;

extern ProportionalTextPrinter StatsTextPrinter;

#endif
