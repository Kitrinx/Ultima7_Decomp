/* Black Gate U7.EXE, overlay segment 258 (file offsets 0x07a870 to 0x07ade8, 1400 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "typefram.h"
#include "itemrec.h"
#include "coord.h"
#include "random.h"
#include "type.h"
#include "item.h"
#include "slime.h"
#include "objref.h"
#include "search.h"

inline int8_t operator==(const Coord &a, const Coord &b)
{
	return a.value == b.value;
}

inline void SetFrame(uint16_t *typeFrame, uint16_t frame)
{
	*typeFrame = (*typeFrame & 0x3ff) | ((frame << 10) & 0xfc00);
}

/* Joins or parts a slime and its neighbour in direction, rerolling its look. A slime's frame bits:
 * 1 picks one of two looks; 2, 4, 8 and 16 join it to the neighbour 2 cells north, east, south and
 * west. */
static void Slime_setJoin(objref ref, uint8_t direction, int8_t enabled)
{
	uint16_t frame = (ref.ptr()->typeFrame & 0x7c00) >> 10;
	int16_t mask = 2 << (direction / 2);

	if (enabled)
		frame |= mask;
	else
		frame &= ~mask;
	if (GenerateRandomIntegerInRange(2))
		frame |= 1;
	else
		frame &= ~1;
	SetFrame(&ref.ptr()->typeFrame, frame & 31);
}

/* Joins a slime at x, y to the slimes beside it (or parts them), and sets its own frame to match. */
static void Slime_joinNeighbors(objref ref, Coord x, Coord y, int8_t enabled)
{
	AreaSearch search;
	uint16_t frame = 0;
	int16_t z = Item_getZ(&ref);
	Coord otherX, otherY;

	FindItemInArea(&search, Coord(x.value - 2), Coord(y.value - 2), Coord(x.value + 2), Coord(y.value + 2), 4, 529,
		255, 255, z, z);  /* slime */
	while (search.found()) {
		Item_getXAndY(search.current, &otherX.value, &otherY.value);
		if (x == otherX && Coord(y.value - 2) == otherY) {
			Slime_setJoin(search.current, 4, enabled);
			frame |= 2;
		}
		if (Coord(x.value + 2) == otherX && y == otherY) {
			Slime_setJoin(search.current, 6, enabled);
			frame |= 4;
		}
		if (x == otherX && Coord(y.value + 2) == otherY) {
			Slime_setJoin(search.current, 0, enabled);
			frame |= 8;
		}
		if (Coord(x.value - 2) == otherX && y == otherY) {
			Slime_setJoin(search.current, 2, enabled);
			frame |= 16;
		}
		FindItem(&search);
	}
	if (GenerateRandomIntegerInRange(2))
		frame |= 1;
	Item_setFrame(&ref, frame);
}

void OnStrangeMoverRemoved(objref ref)
{
	Coord x, y;

	Item_getXAndY(ref, &x.value, &y.value);
	switch (ref.ptr()->typeFrame & 0x3ff) {
	case 529:   /* slime */
		Slime_joinNeighbors(ref, x, y, 0);
		break;
	}
}

void OnStrangeMoverPlaced(objref ref, Coord *x, Coord *y)
{
	switch (ref.ptr()->typeFrame & 0x3ff) {
	case 529:   /* slime */
		Slime_joinNeighbors(ref, *x, *y, 1);
		break;
	}
}

/* A moving slime leaves blood behind it most of the time. */
uint8_t OnStrangeMoverStep(objref ref, Loc, Loc, int16_t)
{
	AreaSearch search;
	Coord x, y;
	int16_t z;

	x.value = Item_getX(ref).value;
	y.value = Item_getY(ref).value;
	z = Item_getZ(&ref);
	switch (ref.ptr()->typeFrame & 0x3ff) {
	case 529:   /* slime */
		if (!(uint8_t)FindItemInArea(&search, x, y, 0, 912, 255, 255)) {     /* blood */
			if (GenerateRandomIntegerInRange(6)) {
				if (CreateItem(&search.current,
					TypeFrame(912 | (((GenerateRandomIntegerInRange(4) + 4) << 10) & 0x7c00)), x, y, z))
					Item_setTemporary(&search.current);
			}
		} else {
			Item_setFrame(&search.current, GenerateRandomIntegerInRange(4) + 4);
		}
		break;
	}
	return 1;
}

uint8_t HasEvenCellPlacement(int16_t type)
{
	switch (type) {
	case 529: return 1;     /* slime */
	default: return 0;
	}
}

uint8_t HasFixedFrames(int16_t type)
{
	switch (type) {
	case 529: return 1;     /* slime */
	default: return 0;
	}
}

int16_t GetStrangeMoverFrame(ItemId id, int16_t value)
{
	objref ref = id.off;
	uint16_t type = ref.ptr()->typeFrame & 0x3ff;

	switch (type) {
	case 529: return -1;    /* slime */
	default: return value;
	}
}

ItemId GetStrangeMoverTarget(ItemId original)
{
	objref ref = original.off;

	if (!(uint8_t)gItemTypeInfo[ref.ptr()->typeFrame & 0x3ff].strangeMovement)
		return original;
	switch (ref.ptr()->typeFrame & 0x3ff) {
	default: return original;
	}
}
