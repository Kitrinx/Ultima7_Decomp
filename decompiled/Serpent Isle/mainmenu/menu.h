#ifndef MENU_H
#define MENU_H

#include "colbuf.h"
#include "mevent.h"

struct Button;
struct Control;
struct Mouse;
struct KeyQueue;
struct MenuEntry;

/* What interpretKey asks of the menu. */
#define MENU_NONE       0
#define MENU_PREVIOUS   1
#define MENU_NEXT       2
#define MENU_SELECT     3

/* Turns a key into one of the MENU_ actions, in place of the arrow and Enter keys. */
typedef char (far *KeyFilter)(int key);

/* A node holding one entry of a menu. */
struct EntryNode : DoubleLink {
	MenuEntry *entry;
	EntryNode() { entry = 0; }
	EntryNode(MenuEntry *e) { entry = e; }
	MenuEntry *getEntry() { return entry; }
	void *operator new(unsigned);
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
	int choice;
	KeyFilter keyFilter;
	MenuEntry(int c, KeyFilter f) { choice = c; keyFilter = f; }
	virtual unsigned char contains(int x, int y) = 0;
	virtual void setHighlight(unsigned char on) = 0;
	virtual unsigned char isLive(unsigned char force) = 0;
	char interpretKey(int key);
	void setKeyFilter(KeyFilter f);
	int getChoice() { return choice; }
};

/* A menu choice shown as a button; a locked one is skipped unless forced. */
struct ButtonEntry : MenuEntry {
	Control *button;
	char locked;
	ButtonEntry(Control *b, int c, KeyFilter f, char l) : MenuEntry(c, f) { button = b; locked = l; }
	unsigned char contains(int x, int y);
	void setHighlight(unsigned char on);
	unsigned char isLive(unsigned char force);
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
	int handleMouse(MouseEvent event);
	int selectAt(int x, int y);
	void showCurrent();
	void select(int choice);
	int run();
	void resetChanged() { changed = 0; }
	char hasChanged() { return changed; }
};

struct ButtonMenu : Menu {
	ButtonMenu() {}
};

void PutPixel(int x, int y, char color);
void AddButtonEntry(Menu *menu, Control *button, int choice, KeyFilter filter, char locked);

#endif
