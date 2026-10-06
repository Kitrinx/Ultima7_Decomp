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
	unsigned char x, y;
	objref object;
	unsigned char active;
};

/* A control in a list of controls. */
struct ControlNode : DoubleLink {
	Control *child;
	ControlNode();
	virtual ~ControlNode();
	ControlNode(Control *);
	Control *get();
	void *operator new(unsigned);
};

struct Point {
	int x, y;
	Point() { x = 0; y = 0; }
	Point(int a, int b) { x = a; y = b; }
};

struct Rectangle : Point {
	int x1, y1;
	Rectangle() : Point() { x1 = 0; y1 = 0; }
	void set(int a, int b, int c, int d) { x = a; y = b; x1 = c; y1 = d; }
	int width() { return x1 - x + 1; }
	int height() { return y1 - y + 1; }
	int contains(int a, int b) { return (x <= a) & (x1 >= a) & (y <= b) & (y1 >= b); }
	void moveTo(int a, int b) { x1 = a + (x1 - x); y1 = b + (y1 - y); x = a; y = b; }
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
	unsigned char visible, locked;
	Control();
	void save();
	void restore();
	unsigned char isVisible();
	unsigned char isLocked();
	void add(Control *);
	void remove(Control *);
	void lower(Control *);
	virtual void paint(View *);
	virtual void moveTo(int, int) = 0;
	virtual unsigned char handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	void show();
	void hide();
	void lock();
	void unlock();
};

struct Sprite : Control {
	int x, y, shape, frame, frameCount;
	Sprite();
	Sprite(int);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
	virtual int getFrame();
	virtual void setFrame(int);
	virtual void setShape(int);
	virtual int getShape();
};

/* A sprite that acts when clicked. */
struct ImageButton : Sprite {
	ImageButton() {}
	ImageButton(int n) : Sprite(n) {}
	unsigned char handle(MouseState *);
};

struct SizedSprite : Sprite {
	int width, height;
	SizedSprite();
	SizedSprite(int);
	unsigned char handle(MouseState *);
	virtual void size(int *, int *);
	virtual void press();
	virtual void release();
};

struct CounterSprite : SizedSprite {
	CounterSprite() { frame = 0; }
	CounterSprite(int n) : SizedSprite(n) { frame = 0; }
	unsigned char handle(MouseState *);
	virtual void advance();
};

struct SpecialSprite : SizedSprite {
	SpecialSprite();
	SpecialSprite(int);
	unsigned char handle(MouseState *);
	virtual void advance();
};

/* Saves the screen under a dragged image. */
struct Draggable {
	int saved, image, hotX, hotY, lastX, lastY;
	Draggable() { saved = -1; }
	virtual ~Draggable();
	virtual void pickUp(int, int) = 0;
	virtual void show(int, int) = 0;
	virtual void drop(int, int) = 0;
	virtual void hide() = 0;
	virtual void dragTo(int, int) = 0;
	virtual void drag(MouseState *) = 0;
	void allocate(int);
	void release();
};

/* A control that holds or shows items. */
struct Panel : Control {
	char dirty;
	virtual void moveTo(int, int) = 0;
	virtual unsigned char handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	virtual objref object() = 0;
	virtual unsigned char accepts(objref, int, int) = 0;
	virtual objref selected() = 0;
	virtual int mouseX() = 0;
	virtual int mouseY() = 0;
	virtual int dragX() = 0;
	virtual int dragY() = 0;
	virtual void setDragX(int) = 0;
	virtual void setDragY(int) = 0;
	virtual void refresh(char) = 0;
};

/* The items a container shows, with their places. */
struct ItemList : DoubleList {
	void append(objref, unsigned char, unsigned char);
	void prepend(objref, unsigned char, unsigned char);
	void markStale();
	ItemNode *find(objref);
	void removeStale();
};

struct ItemGrid : Control {
	Rectangle region;
	objref container;
	ItemList items;
	objref selectedItem;
	int clickX, clickY, offsetX, offsetY;
	ItemGrid() {}
	void initialize(objref, unsigned char, unsigned char, unsigned char, unsigned char);
	void refresh();
	void place(objref, unsigned char *, unsigned char *, unsigned char *);
	void build(objref);
	ItemNode *find(int, int);
	unsigned char handle(MouseState *);
	void moveTo(int, int);
	void draw(View *);
	unsigned char accepts(objref, int, int);
	virtual objref object();
};

/* An item dialog: a container, a paperdoll, a spellbook or a stats sheet. */
struct Gump : Draggable, Panel {
	objref displayed;
	int kind, shape;
	SizedSprite closeButton;
	Rectangle bounds;
	unsigned char stats;
	objref owner;
	unsigned char dragging;
	Gump() : closeButton(2) { stats = 0; dragging = 0; dirty = 0; }
	~Gump();
	void pickUp(int, int);
	void show(int, int);
	void drop(int, int);
	void hide();
	void dragTo(int, int);
	void drag(MouseState *);
	virtual unsigned char handle(MouseState *) = 0;
	virtual void draw(View *) = 0;
	virtual void moveTo(int, int) = 0;
	virtual unsigned char accepts(objref, int, int) = 0;
	virtual objref selected() = 0;
	virtual int mouseX() = 0;
	virtual int mouseY() = 0;
	virtual int dragX() = 0;
	virtual int dragY() = 0;
	virtual void setDragX(int) = 0;
	virtual void setDragY(int) = 0;
	virtual void refresh(char) = 0;
	objref object();
	virtual unsigned char findPosition(objref, int *, int *) = 0;
};

struct ContainerGump : Gump {
	ItemGrid contents;
	ContainerGump(objref);
	~ContainerGump();
	void initialize();
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};

struct StatsGump : Gump {
	objref displayedItem;
	int clickX, clickY;
	char unusedField1[65];
	Sprite asleepIcon, poisonedIcon, charmedIcon, hungryIcon, protectedIcon, cursedIcon, paralyzedIcon;
	int frame;
	int dragOffsetX, dragOffsetY;
	StatsGump(objref);
	void initialize();
	void update();
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};

struct InventoryGump;

/* One equipment slot of the paperdoll. */
struct ItemSlot : Control {
	int x, y;
	objref carried;
	unsigned char male;         /* whether the paperdoll is male */
	unsigned char showEmpty;    /* drawn even with nothing in it */
	int shape;                  /* paperdoll shape for the item, 0 to draw the item itself */
	unsigned char frame;
	ItemSlot() {}
	void initialize(objref, unsigned char, unsigned char, InventoryGump *);
	unsigned char handle(MouseState *);
	void moveTo(int, int);
	void draw(View *);
	virtual objref object();
	unsigned char setItem(objref, unsigned char, InventoryGump *);
	void pickFrame(unsigned char, InventoryGump *);
};

/* the paperdoll's slots */
#define SLOT_COUNT 18

struct InventoryGump : Gump {
	ItemSlot slots[SLOT_COUNT];
	objref selectedItem;
	int clickX, clickY;
	char unusedByte;
	char weaponPose;            /* 0 one hand, 1 two hands, 2 a staff */
	SizedSprite statsButton, diskButton;
	CounterSprite combatButton;
	Sprite leftHand, rightHand;           /* drawn at a hand holding an item with no paperdoll picture */
	ImageButton torso, head, legs, neck, arms, feet, quiverAmmo;
	SizedSprite combatStatsButton;
	int dragOffsetX, dragOffsetY;
	InventoryGump(NPCRef);
	~InventoryGump();
	void initialize();
	void update(NPCRef, unsigned char, unsigned char);
	unsigned char findSlot(unsigned char *, objref, int, int);
	unsigned char ammoLoaded();
	void poseArms();
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};


/* The party's faces, health and combat modes. */
struct CombatStatsGump : Gump {
	int clickX, clickY;
	ImageButton faces[6];
	SpecialSprite attackModeButtons[6];
	CounterSprite protectButtons[6];
	int dragOffsetX, dragOffsetY;
	CombatStatsGump();
	void initialize();
	void update();
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};

/* One tooth's place in the serpent jawbone. */
struct ToothSlot : Control {
	int shape, frame;
	int x, y;
	objref tooth;
	ToothSlot() {}
	void initialize(objref);
	unsigned char handle(MouseState *);
	void moveTo(int, int);
	void draw(View *);
	unsigned char setTooth(objref);
	virtual objref object();
};

/* the jawbone's teeth */
#define TOOTH_COUNT 18

/* The serpent jawbone, holding up to eighteen teeth. */
struct JawboneGump : Gump {
	ItemGrid contents;
	ToothSlot slots[TOOTH_COUNT];
	objref selectedItem;
	int clickX, clickY;
	int dragOffsetX, dragOffsetY;
	JawboneGump(objref);
	~JawboneGump();
	void initialize();
	void update(unsigned char);
	void refresh(char);
	unsigned char handle(MouseState *);
	void draw(View *);
	void moveTo(int, int);
	unsigned char toothSlot(unsigned char *, objref);
	unsigned char accepts(objref, int, int);
	objref selected();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	int mouseX();
	int mouseY();
	unsigned char findPosition(objref, int *, int *);
};

/* A magic scroll: the picture its quality chooses. */
struct SpellScrollGump : Gump {
	int clickX, clickY;
	char unusedBytes[7];
	ImageButton picture;
	int dragOffsetX, dragOffsetY;
	SpellScrollGump(objref);
	void initialize();
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char);
	unsigned char findPosition(objref, int *, int *);
};

/* An item carried by the mouse. */
struct ItemDrag : Draggable, Control {
	objref obj;
	int posx, posy;
	ItemDrag(objref, int, int, int, int);
	void pickUp(int, int);
	void show(int, int);
	void drop(int, int);
	void hide();
	void dragTo(int, int);
	void drag(MouseState *);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
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
#define GUMP_COMBAT_STATS   18
#define GUMP_OWNER          19  /* reopen the gump this one was opened from */
#define BUTTON_CLICKED      20
#define BUTTON_DOUBLE_CLICK 21
#define GUMP_CAST_SPELL     22
#define GUMP_READ_SCROLL    23  /* cast the spell a scroll holds, using it up */
#define BUTTON_ON           24  /* a two-state button turned on */
#define BUTTON_OFF          25
#define GUMP_RESTORED       26  /* a saved game was loaded */
#define GUMP_SELECT_ITEM    27  /* the item clicked is picked, or its name shown */
#define GUMP_USE_ITEM       28
#define GUMP_QUIT           29
#define GUMP_YES            32
#define GUMP_NO             33
#define GUMP_REFRESH        34  /* combat began: update the inventories */
#define GUMP_PICK_GROUND    35  /* picking, and nothing lies under the mouse */

struct ProportionalTextPrinter;

extern ProportionalTextPrinter StatsTextPrinter;

#endif
