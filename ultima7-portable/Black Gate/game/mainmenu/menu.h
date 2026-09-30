#ifndef MAINMENU_MENU_H
#define MAINMENU_MENU_H

#include "colbuf.h"
#include "mevent.h"

struct Mouse;

namespace MainMenu {

struct Button;
struct KeyQueue;
struct MenuEntry;

/* What interpretKey asks of the menu. */
#define MENU_NONE       0
#define MENU_PREVIOUS   1
#define MENU_NEXT       2
#define MENU_SELECT     3

/* Turns a key into one of the MENU_ actions, in place of the arrow and Enter keys. */
typedef char (*KeyFilter)(int16_t key);

/* A node holding one entry of a menu. */
struct EntryNode : DoubleLink {
	MenuEntry *entry;
	EntryNode() { entry = 0; }
	EntryNode(MenuEntry *e) { entry = e; }
};

/* A menu's entries, in the order the arrow keys visit them. */
struct EntryList : DoubleList {
	EntryList() : DoubleList() {}
	~EntryList() { List_removeAndDestroyAll(this); }
	void append(MenuEntry *entry);
	void prepend(MenuEntry *entry);
	EntryNode *find(MenuEntry *entry);
};

/* One choice on a menu. */
struct MenuEntry {
	int16_t choice;
	KeyFilter keyFilter;
	MenuEntry(int16_t c, KeyFilter f) { choice = c; keyFilter = f; }
	virtual ~MenuEntry() {}
	virtual uint8_t contains(int16_t x, int16_t y) = 0;
	virtual void setHighlight(uint8_t on) = 0;
	virtual uint8_t isLive(uint8_t force) = 0;
	char interpretKey(int16_t key);
	void setKeyFilter(KeyFilter f);
};

/* A menu choice shown as a button; a locked one is skipped unless forced. */
struct ButtonEntry : MenuEntry {
	Button *button;
	char locked;
	ButtonEntry(Button *b, int16_t c, KeyFilter f, char l) : MenuEntry(c, f) { button = b; locked = l; }
	uint8_t contains(int16_t x, int16_t y);
	void setHighlight(uint8_t on);
	uint8_t isLive(uint8_t force);
};

/* The choices on a screen, stepped through with the keys or picked with the mouse. */
struct Menu {
	Mouse *pointer;
	KeyQueue *keys;
	EntryList entries;
	MenuEntry *current;
	char keysActive;
	char changed;
	Menu() { pointer = 0; current = 0; keysActive = 1; keys = 0; changed = 0; }
	void add(MenuEntry *entry);
	void remove(MenuEntry *entry);
	void setPointer(Mouse *mouse);
	void setKeys(KeyQueue *queue);
	int16_t handleMouse(MouseEvent event);
	int16_t selectAt(int16_t x, int16_t y);
	void showCurrent();
	void select(int16_t choice);
	int16_t run();
};

void AddButtonEntry(Menu *menu, Button *button, int16_t choice, KeyFilter filter, char locked);

}

#endif
