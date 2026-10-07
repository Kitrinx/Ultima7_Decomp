/* Serpent Isle MAINMENU.EXE, resident segment 7 (file offsets 0x00c658 to 0x00c769, 273 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "menu.h"
#include "controls.h"

namespace MainMenu {

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

void AddButtonEntry(Menu *menu, Control *button, int16_t choice, KeyFilter filter, int8_t locked)
{
	MenuEntry *entry;

	entry = new ButtonEntry(button, choice, filter, locked);
	menu->add(entry);
}

void NullControl(void)
{
}

}
