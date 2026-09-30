/* Black Gate U7.EXE, overlay segment 250 (file offsets 0x076450 to 0x076bef, 1951 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "iteminfo.h"
#include "item.h"
#include "coord.h"
#include "voolook.h"
#include "makemojo.h"
#include "script.h"
#include "actqueue.h"
#include "random.h"
#include "explode.h"
#include "search.h"

#define FRAME(ref) ((ITEM((ref).off)->typeFrame & 0x7c00) >> 10)

struct Script {
	uint8_t length;
	char data[127];
	Script() { length = 1; }
};

uint8_t PowderKegExploding = 0;

extern objref AvatarRef;

/* Sets off every powder keg within 5 cells, each after a delay by distance, and blows away about
 * half the doors within 6. */
void ExplodePowderKeg(objref source)
{
	objref removed;
	int16_t weapon, x, y, z;
	AreaSearch items;

	PowderKegExploding = 1;
	weapon = WeaponLookup.get(704);     /* powder keg */
	Item_setQuality(&source, 1);
	FindItemInArea(&items,
		Coord(Item_getX(source) - 5), Coord(Item_getY(source) - 5),
		Coord(Item_getX(source) + 5), Coord(Item_getY(source) + 5),
		0, 704, 0, 255);    /* powder keg */
	while (items.found()) {
		int16_t delay = Item_greatestDeltaToItem(source, items.current) + 2;
		Script command;

		AppendScriptByte(&command.length, SCRIPT_USECODE);
		AppendScriptWord(&command.length, 704);     /* the powder keg's usable */
		Item_setQuality(&items.current, 1);
		ActionQueue.add(delay, items.current.off, (char *)&command);
		FindItem(&items);
	}
	FindItemInArea(&items,
		Coord(Item_getX(source) - 6), Coord(Item_getY(source) - 6),
		Coord(Item_getX(source) + 6), Coord(Item_getY(source) + 6),
		0, 432, 255, 255);  /* door */
	while (items.found()) {
		uint16_t frame;

		removed = 0;
		frame = FRAME(items.current);
		if (GenerateRandomIntegerInRange(2) &&
			((frame >= 0 && frame <= 2) || (frame >= 4 && frame <= 6))) {
			removed = items.current.off;
			FindItem(&items);
			Item_delete(&removed);
		} else {
			FindItem(&items);
		}
	}
	FindItemInArea(&items,
		Coord(Item_getX(source) - 6), Coord(Item_getY(source) - 6),
		Coord(Item_getX(source) + 6), Coord(Item_getY(source) + 6),
		0, 433, 255, 255);  /* door */
	while (items.found()) {
		uint16_t frame;

		removed = 0;
		frame = FRAME(items.current);
		if (GenerateRandomIntegerInRange(2) &&
			((frame >= 0 && frame <= 2) || (frame >= 4 && frame <= 6))) {
			removed = items.current.off;
			FindItem(&items);
			Item_delete(&removed);
		} else {
			FindItem(&items);
		}
	}
	FindItemInArea(&items,
		Coord(Item_getX(source) - 6), Coord(Item_getY(source) - 6),
		Coord(Item_getX(source) + 6), Coord(Item_getY(source) + 6),
		0, 270, 255, 255);  /* door */
	while (items.found()) {
		uint16_t frame;

		removed = 0;
		frame = FRAME(items.current);
		if (GenerateRandomIntegerInRange(2) &&
			((frame >= 0 && frame <= 2) || (frame >= 8 && frame <= 10) || (frame >= 16 && frame <= 18))) {
			removed = items.current.off;
			FindItem(&items);
			Item_delete(&removed);
		} else {
			FindItem(&items);
		}
	}
	FindItemInArea(&items,
		Coord(Item_getX(source) - 6), Coord(Item_getY(source) - 6),
		Coord(Item_getX(source) + 6), Coord(Item_getY(source) + 6),
		0, 376, 255, 255);  /* door */
	while (items.found()) {
		uint16_t frame;

		removed = 0;
		frame = FRAME(items.current);
		if (GenerateRandomIntegerInRange(2) &&
			((frame >= 0 && frame <= 2) || (frame >= 8 && frame <= 10) || (frame >= 16 && frame <= 18))) {
			removed = items.current.off;
			FindItem(&items);
			Item_delete(&removed);
		} else {
			FindItem(&items);
		}
	}
	x = Item_getX(source);
	y = Item_getY(source);
	z = Item_getZ(&source);
	Item_delete(&source);
	Explode(AvatarRef, x, y, z, weapon, 0, 0);
	PowderKegExploding = 0;
}
