/* Serpent Isle SI.EXE, resident segment 103 (file offsets 0x036ed3 to 0x037bf5, 3362 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
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
	: asleepIcon(23), poisonedIcon(23), charmedIcon(23), hungryIcon(23), protectedIcon(23), cursedIcon(23),
	paralyzedIcon(23)
{
	displayedItem = item;
	initialize();
}

InventoryGump::InventoryGump(NPCRef item)
	: statsButton(20), diskButton(19), combatButton(41), leftHand(55), rightHand(54), torso(0), head(0),
	legs(0), neck(0), arms(0), feet(0), quiverAmmo(0), combatStatsButton(91)
{
	displayed = item;
	initialize();
}

InventoryGump::~InventoryGump() {}
