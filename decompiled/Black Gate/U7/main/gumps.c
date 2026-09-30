/* Black Gate U7.EXE, resident segment 116 (file offsets 0x039726 to 0x03a278, 2898 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "item.h"
#include "colbuf.h"
#include "npcref.h"
#include "gumps.h"

ContainerGump::~ContainerGump() {}

Gump::~Gump() {}

objref Gump::object() { return displayed; }

void ItemDrag::draw(View *) {}

objref ItemDrag::object() { return obj; }

unsigned char ItemDrag::handle(MouseState *) { return 0; }

objref ItemSlot::object() { return carried; }

ContainerGump::ContainerGump(objref item)
{
	displayed = item;
	initialize();
}

ItemDrag::ItemDrag(objref item, int x, int y, int grabX, int grabY)
{
	Draggable::hotX = grabX;
	Draggable::hotY = grabY;
	obj = item;
	moveTo(x - Draggable::hotX, y - Draggable::hotY);
}

StatsGump::StatsGump(objref item)
	: asleepIcon(28), poisonedIcon(28), charmedIcon(28), hungryIcon(28), protectedIcon(28), cursedIcon(28),
	paralyzedIcon(28)
{
	displayedItem = item;
	initialize();
}

InventoryGump::InventoryGump(NPCRef item)
	: statsButton(25), diskButton(24), combatButton(46), twoSlotMark(48), twoHandedMark(48), attackModeButton(12),
	protectButton(7)
{
	displayed = item;
	initialize();
}

InventoryGump::~InventoryGump() {}
