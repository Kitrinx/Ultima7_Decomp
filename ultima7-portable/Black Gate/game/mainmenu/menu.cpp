/* Black Gate MAINMENU.EXE modules MENU and MENUENT: choices picked with the mouse or stepped
 * through with the arrow keys.
 */

#include "u7port.h"
#include "u7event.h"
#include "view.h"
#include "keyqueue.h"
#include "controls.h"
#include "menu.h"

namespace MainMenu {

#define KEY_ENTER       13

void EntryList::append(MenuEntry *entry)
{
	EntryNode *node = new EntryNode(entry);

	List_insertAtTail(this, node);
}

void EntryList::prepend(MenuEntry *entry)
{
	EntryNode *node = new EntryNode(entry);

	List_insertAtHead(this, node);
}

EntryNode *EntryList::find(MenuEntry *entry)
{
	EntryNode *node = 0;

	if (entry)
		while (List_stepForward(this, (DoubleLink **) &node))
			if (node->entry == entry)
				break;
	return node;
}

char MenuEntry::interpretKey(int16_t key)
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

void Menu::add(MenuEntry *entry)
{
	entries.append(entry);
	current = entry;
}

void Menu::remove(MenuEntry *entry)
{
	EntryNode *node = 0;

	while (List_stepForward(&entries, (DoubleLink **) &node))
		if (node->entry == entry) {
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
int16_t Menu::handleMouse(MouseEvent event)
{
	return selectAt(event.x >> 1, event.y);
}

/* Selects the last live entry under (x, y); true when there is one. */
int16_t Menu::selectAt(int16_t x, int16_t y)
{
	EntryNode *node = 0;

	do {
		if (List_stepBackward(&entries, (DoubleLink **) &node)) {
			MenuEntry *entry = node->entry;
			if (entry->isLive(1) && entry->contains(x, y))
				break;
		}
	} while (node != 0);
	if (node != 0)
		current = node->entry;
	else
		current = 0;
	return current != 0;
}

/* Highlight the current entry and no other. */
void Menu::showCurrent()
{
	EntryNode *node = 0;

	while (List_stepForward(&entries, (DoubleLink **) &node)) {
		if (node->entry == current)
			node->entry->setHighlight(1);
		else
			node->entry->setHighlight(0);
	}
}

void Menu::select(int16_t choice)
{
	EntryNode *node = 0;
	MenuEntry *entry = 0;

	while (List_stepForward(&entries, (DoubleLink **) &node)) {
		entry = node->entry;
		if (entry->choice == choice)
			break;
		entry = 0;
	}
	current = entry;
	showCurrent();
}

/* One pass of the menu: follows the mouse or the arrow keys, and returns the choice picked, or 0. */
int16_t Menu::run()
{
	char picked = 0;
	MouseEvent event;

	CopyMouseEvent(&event);
	if (event.type != 0) {
		changed = 1;
		switch (event.type) {
		case MOUSE_EVENT_RELEASED:
			picked = handleMouse(event);
			break;
		case MOUSE_EVENT_MOVED:
			handleMouse(event);
			break;
		}
	} else if (keysActive) {
		int16_t key;
		uint8_t action;
		EntryNode *node;
		EntryNode *link;

		if (keys->peek(&key)) {
			changed = 1;
			node = entries.find(current);
			link = 0;
			if (node == 0) {
				if (List_stepForward(&entries, (DoubleLink **) &node)) {
					while (node) {
						MenuEntry *entry = node->entry;
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
						MenuEntry *entry = link->entry;
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
						MenuEntry *entry = link->entry;
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
	return picked ? current->choice : 0;
}

uint8_t ButtonEntry::contains(int16_t x, int16_t y)
{
	return button->contains(x, y);
}

void ButtonEntry::setHighlight(uint8_t on)
{
	button->setHighlight(on);
}

uint8_t ButtonEntry::isLive(uint8_t force)
{
	if (locked && !force)
		return 0;
	return button->isVisible();
}

void AddButtonEntry(Menu *menu, Button *button, int16_t choice, KeyFilter filter, char locked)
{
	MenuEntry *entry;

	entry = new ButtonEntry(button, choice, filter, locked);
	menu->add(entry);
}

}
