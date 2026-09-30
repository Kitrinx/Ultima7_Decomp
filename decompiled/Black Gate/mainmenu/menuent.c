/* Black Gate MAINMENU.EXE, resident segment 7 (file offsets 0x00bea5 to 0x00bf58, 179 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "menu.h"
#include "controls.h"

unsigned char ButtonEntry::contains(int x, int y)
{
	return button->contains(x, y);
}

void ButtonEntry::setHighlight(unsigned char on)
{
	button->setHighlight(on);
}

unsigned char ButtonEntry::isLive(unsigned char force)
{
	if (locked && !force)
		return 0;
	return button->isVisible();
}

void AddButtonEntry(Menu *menu, Button *button, int choice, KeyFilter filter, char locked)
{
	MenuEntry *entry;

	entry = new ButtonEntry(button, choice, filter, locked);
	menu->add(entry);
}

void NullControl(void)
{
}
