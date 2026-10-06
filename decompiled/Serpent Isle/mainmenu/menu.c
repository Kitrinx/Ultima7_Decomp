/* Serpent Isle MAINMENU.EXE, resident segment 6 (file offsets 0x00c030 to 0x00c658, 1576 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "u7event.h"
#include "oops.h"
#include "keyqueue.h"
#include "menu.h"

#define KEY_ENTER       13
#define SCREEN_WIDTH    320
#define VIDEO_SEGMENT   0xA000

void *EntryNode::operator new(unsigned)
{
	EntryNode *node = ::new EntryNode;

	if (!node)
		ReportOutOfNearMemory();
	return node;
}

void EntryList::append(MenuEntry *entry)
{
	EntryNode *node = new EntryNode(entry);

	node->entry = entry;
	List_insertAtTail(this, node);
}

void EntryList::prepend(MenuEntry *entry)
{
	EntryNode *node = new EntryNode(entry);

	node->entry = entry;
	List_insertAtHead(this, node);
}

EntryNode *EntryList::find(MenuEntry *entry)
{
	EntryNode *node = 0;

	if (entry)
		while (List_stepForward(this, (DoubleLink **) &node))
			if (node->getEntry() == entry)
				break;
	return node;
}

char MenuEntry::interpretKey(int key)
{
	char action = MENU_NONE;

	if (keyFilter) {
		action = keyFilter(key);
	} else {
		switch (key) {
		case KEY_DOWN:
			action = MENU_NEXT;
			break;
		case KEY_UP:
			action = MENU_PREVIOUS;
			break;
		case KEY_ENTER:
			action = MENU_SELECT;
			break;
		}
	}
	return action;
}

void MenuEntry::setKeyFilter(KeyFilter f)
{
	keyFilter = f;
}

void PutPixel(int x, int y, char color)
{
	char far *screen = (char far *) MK_FP(VIDEO_SEGMENT, x + y * SCREEN_WIDTH);

	*screen = color;
}

void Menu::add(MenuEntry *entry)
{
	entries.append(entry);
	current = entry;
}

void Menu::remove(MenuEntry *entry)
{
	EntryNode *node = 0;

	while (List_stepForward(&entries, (DoubleLink **) &node))
		if (node->getEntry() == entry) {
			List_unlink(&entries, node);
			return;
		}
}

void Menu::setPointer(Mouse *mouse)
{
	pointer = mouse;
}

void Menu::setKeys(KeyQueue *queue)
{
	keys = queue;
}

/* The mouse works in doubled x. */
int Menu::handleMouse(MouseEvent event)
{
	return selectAt(event.getX() >> 1, event.getY());
}

/* Selects the last live entry under (x, y); true when there is one. */
int Menu::selectAt(int x, int y)
{
	EntryNode *node = 0;

	do {
		if (List_stepBackward(&entries, (DoubleLink **) &node)) {
			MenuEntry *entry = node->getEntry();
			if (entry->isLive(1) && entry->contains(x, y))
				break;
		}
	} while (node != 0);
	if (node != 0)
		current = node->getEntry();
	else
		current = 0;
	return current != 0;
}

/* Highlight the current entry and no other. */
void Menu::showCurrent()
{
	EntryNode *node = 0;

	while (List_stepForward(&entries, (DoubleLink **) &node)) {
		if (node->getEntry() == current)
			node->getEntry()->setHighlight(1);
		else
			node->getEntry()->setHighlight(0);
	}
}

void Menu::select(int choice)
{
	EntryNode *node = 0;
	MenuEntry *entry;

	while (List_stepForward(&entries, (DoubleLink **) &node)) {
		entry = node->getEntry();
		if (entry->getChoice() == choice)
			break;
		entry = 0;
	}
	current = entry;
	showCurrent();
}

/* One pass of the menu: follows the mouse or the arrow keys, and returns the choice picked, or 0. */
int Menu::run()
{
	char picked = 0;
	MouseEvent event;

	CopyMouseEvent(&event);
	if (event.valid()) {
		changed = 1;
		switch (event.getType()) {
		case MOUSE_EVENT_RELEASED:
			picked = handleMouse(event);
			break;
		case MOUSE_EVENT_MOVED:
			handleMouse(event);
			break;
		}
	} else if (keysActive) {
		int key;
		unsigned char action;
		EntryNode *node;
		EntryNode *link;

		if (keys->peek(&key)) {
			changed = 1;
			node = entries.find(current);
			link = 0;
			if (node == 0) {
				if (List_stepForward(&entries, (DoubleLink **) &node)) {
					while (node) {
						MenuEntry *entry = node->getEntry();
						if (entry->isLive(0)) {
							current = entry;
							break;
						}
						node = (EntryNode *) node->next;
					}
				}
			} else {
				action = current->interpretKey(key);
				switch (action) {
				case MENU_NEXT:
					keys->get(&key);
					if (node)
						link = (EntryNode *) node->next;
					while (link) {
						MenuEntry *entry = link->getEntry();
						if (entry->isLive(0)) {
							current = entry;
							break;
						}
						link = (EntryNode *) link->next;
					}
					break;
				case MENU_PREVIOUS:
					keys->get(&key);
					if (node)
						link = (EntryNode *) node->prev;
					while (link) {
						MenuEntry *entry = link->getEntry();
						if (entry->isLive(0)) {
							current = entry;
							break;
						}
						link = (EntryNode *) link->prev;
					}
					break;
				case MENU_SELECT:
					keys->get(&key);
					picked = 1;
					break;
				}
			}
		}
	}
	showCurrent();
	return picked ? current->getChoice() : 0;
}
