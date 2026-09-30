/* Black Gate U7.EXE, overlay segment 214 (file offsets 0x053af0 to 0x054aad, 4029 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdio.h>
#include "lowlevel.h"
#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "npcref.h"
#include "type.h"
#include "u7npc.h"
#include "cheat.h"
#include "coord.h"
#include "colbuf.h"
#include "gumpmgr.h"
#include "makemojo.h"
#include "camera.h"
#include "partymov.h"
#include "voolook.h"
#include "u7manage.h"
#include "cast.h"
#include "crime.h"
#include "equip.h"
#include "itemovr1.h"
#include "party.h"
#include "search.h"
#include "target.h"
#include "weight.h"
#include "random.h"
#include "itable.h"
#include "debug.h"
#include "u7map.h"
#include "wihh.h"
#include "ready.h"
#include "bogus.h"
#include "plat.h"
#include "arena.h"
#include "vidmode.h"

extern objref Item_getContainer(objref *);
extern objref GetContainedItem(objref *);

struct Rect : Point {
	int16_t x1, y1;
	Rect(int16_t a, int16_t b, int16_t c, int16_t d) : Point(a, b) { x1 = c; y1 = d; }
	int16_t contains(int16_t px, int16_t py) { return (px >= x) & (px <= x1) & (py >= y) & (py <= y1); }
};

inline uint8_t GetEquipmentSlot(uint16_t shape) { return ReadyRecords.get(ReadyLookup.get(shape))->slot; }

extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern uint8_t Item_getQuantity(objref *ref);
extern uint8_t Item_isOkayToTake(objref *);
extern void Item_setOkayToTake(objref *);
extern int16_t GetItemBeingDragged(objref *ref);
extern uint8_t ZapDetachedItem(objref *ref);
extern uint8_t CreateItem(objref *, TypeFrame);
extern void Item_setFrame(objref *, int16_t);
extern void Item_setQuality(objref *, int8_t);
extern void Item_setTemporary(objref *);
extern int8_t Item_delete(objref *ref);
extern uint8_t Item_getQualityFlags(objref *);
extern void Item_setQualityFlags(objref *, uint8_t);
extern "C" void PlaySfx(uint8_t number, int16_t volume, int16_t pan, int16_t flags);

int16_t EarthquakeCount;
uint8_t StompCounter = 0;

void EmptyRoutine()
{
}

void PickWorldCoords(int16_t x, int16_t y, Coord *wx, Coord *wy, int16_t *wz, int8_t flag)
{
	objref none = 0;
	PickWorldCoordsOnItem(x, y, wx, wy, wz, &none, flag);
}

void PickWorldCoordsOnItem(int16_t x, int16_t y, Coord *wx, Coord *wy, int16_t *wz, objref *target, int8_t flag)
{
	if (!target->valid())
		FindItemAtScreenPoint(target, x, y, &MainWorldView, &gShapeManager, 1);
	if (target->valid()) {
		int16_t unusedType = target->type();
		int16_t rise = (uint8_t)(target->z() + gItemTypeInfo[target->type()].height) * 4;
		int16_t w = GetFootprintX(*(TypeFrame *)&target->ptr()->typeFrame);
		int16_t h = GetFootprintY(*(TypeFrame *)&target->ptr()->typeFrame);
		int16_t tx, ty;
		WorldCoordsToScreen(Item_getX(*target), Item_getY(*target), &tx, &ty);
		int16_t x0 = tx - w * 8 - rise - 8;
		int16_t y0 = ty - h * 8 - rise - 8;
		int16_t x1 = tx - rise;
		int16_t y1 = ty - rise;
		Rect r(x0, y0, x1, y1);
		if (!r.contains(x, y)) {
			if (x < x0)
				x = x0;
			else if (x > x1)
				x = x1;
			if (y < y0)
				y = y0;
			else if (*wy > CellCoord(y1))     /* tests *wy, as shipped */
				y = y1;
		}
		ScreenToWorldCoords(x + rise, y + rise, &wx->value, &wy->value, flag);
		*wz = (uint8_t)(target->z() + gItemTypeInfo[target->type()].height);
	} else {
		ScreenToWorldCoords(x, y, &wx->value, &wy->value, flag);
		*wz = 0;
	}
}

void CheatCastSpell()
{
	int16_t spell;
	objref selected;
	Coord x, y;
	int16_t z;
	DebugPrintfAtCoords(1, 1, "Enter the spell number (0-79) :");
	spell = (int16_t) ConsoleReadNumber(0);
	HavePlayerSelect(&selected, &x, &y, &z);
	if (selected.isBody())
		TryToCastSpell(selected, spell, 0, 0, 1);
}

int16_t CountHeldItems(int8_t allParty, objref container, int16_t type, int16_t quality, int16_t frame)
{
	int16_t quantity = 0;
	AreaSearch found;
	if (allParty) {
		for (int16_t i = PartySize - 1; i >= 0; i--) {
			if (FindItemInContainer(&found, PartyMembers[i], 0, type, quality, frame)) {
				do {
					if (Item_getContainer(&found.current).type() != 522 && /* locked chest */
						Item_getContainer(&found.current).type() != 798)   /* sealed box */
						quantity += Item_getQuantity(&found.current);
				} while (FindItem(&found));
			}
		}
	} else {
		if (FindItemInContainer(&found, container, 0, type, quality, frame)) {
			do {
				if (Item_getContainer(&found.current).type() != 522 && /* locked chest */
					Item_getContainer(&found.current).type() != 798)   /* sealed box */
					quantity += Item_getQuantity(&found.current);
			} while (FindItem(&found));
		}
	}
	return quantity;
}

void MarkItemOkayToTake(objref item)
{
	AreaSearch found;
	if (!Item_isOkayToTake(&item)) {
		if (!HackMoverEnabled && TheftReported != 0) {
			ReportCrime(item.off, 1, 1);
			TheftReported = 1;
		}
		Item_setOkayToTake(&item);
		if (FindItemInContainer(&found, item, 0, -1, 255, 255)) {
			do {
				Item_setOkayToTake(&found.current);
			} while (FindItem(&found));
		}
	}
}

int16_t TryToPlaceItem(objref target, uint8_t combine, uint8_t mark)
{
	objref dragged, occupied, container, npc;
	uint8_t inBody = 0;
	int16_t slot = -1;
	int16_t error = 0;
	GetItemBeingDragged(&dragged);
	if (target.isBody()) {
		npc = target;
		container = 0;
		if ((uint8_t)((GetNpcBufferForIbo(&npc)->status & NPC_IN_PARTY) != 0) && mark)
			Item_setOkayToTake(&dragged);
	} else {
		container = target;
		npc = 0;
	}
	if (npc.valid()) {
		int16_t state = 1;
		while (state) {
			switch (state) {
			case 1:
				switch (slot = GetEquipmentSlot(dragged.type())) {
				case 0:
				case 1:
					slot = 1;
					state = 3;
					break;
				case 20:
					state = 4;
					break;
				case 6:
					slot = 6;
					state = 6;
					break;
				default:
					state = 2;
				}
				break;
			case 2:
				slot = 1;
				state = 3;
				break;
			case 3:
				slot = 2;
				state = 4;
				break;
			case 4:
				slot = 0;
				state = 5;
				break;
			case 5:
				state = 0;
				break;
			case 6:
				slot = 7;
				state = 2;
				break;
			}
			if (state) {
				occupied = GetItemInSlot(target, slot);
				if (CanEquipInSlot(dragged, target, slot, combine)) {
					container = target;
					npc = target;
					state = 0;
				} else if (occupied.isContainer() && !container.valid()) {
					int16_t type = occupied.type();
					/* not a locked chest or sealed box */
					if (type != 522 && type != 798 && CanHoldBulk(occupied, dragged)) {
						/* 553 firedoom staff, 551 fire sword */
						if (!(uint8_t)gItemTypeInfo[dragged.type()].light || dragged.type() == 553
							|| dragged.type() == 551) {
							container = occupied;
							npc = 0;
						}
					}
				}
			}
		}
	}
	if (container.valid()) {
		objref outer;
		outer = GetOuterContainer(container, dragged);
		if (outer == dragged) {
			error = 2;
		} else if (outer.isBody()) {
			inBody = 1;
			if (!CanCarryWeight(outer, dragged))
				error = 1;
		}
		if (error)
			return error;
		if (!container.isBody()) {
			int16_t type = container.type();
			if (type == 522 || type == 798) {   /* locked chest, sealed box */
				error = 3;
			} else if (!CanHoldBulk(container, dragged)) {
				error = 4;
			} else if ((uint8_t)gItemTypeInfo[dragged.type()].light) {
				int16_t type = dragged.type();
				if (type != 553 && type != 551)     /* firedoom staff, fire sword */
					return 3;
			}
			if (error)
				return error;
			if (combine) {
				objref child = GetContainedItem(&container);
				while (child.valid()) {
					if (CanStackWith(&child, dragged)) {
						Item_setQuantity(child, Item_getQuantity(&child) + Item_getQuantity(&dragged), 0);
						ZapDetachedItem(&dragged);
						return 0;
					}
					child = objref(child.ptr()->next);
				}
			}
		}
		if (PlaceDraggedInContainer(&dragged, container)) {
			if (npc.valid()) {
				uint8_t unusedEquipped = EquipItem(dragged, target, slot, combine);
			}
			if (inBody)
				MarkItemOkayToTake(dragged);
			return 0;
		}
		error = 3;
	} else {
		error = 3;
	}
	return error;
}

uint16_t GiveItemsToParty(int16_t count, int16_t type, int16_t quality, int16_t frame, int16_t temporary)
{
	objref item;
	int16_t member = 0;
	uint16_t placedMask = 0;
	int16_t party;
	TypeFrame typeFrame = 0;
	int16_t unusedPlaced;
	int16_t amount;
	typeFrame.setType(type);
	typeFrame.setFrame(ClampShapeFrame(type, frame));
	party = PartySize;
	amount = 1;
	unusedPlaced = 0;
	while (count) {
		if (gItemTypeInfo[typeFrame.type()].typeClass == TYPE_CLASS_QUANTITY)
			amount = count > 100 ? 100 : count;
		if (CreateItem(&item, typeFrame)) {
			Item_setFrame(&item, frame);
			Item_setQuality(&item, quality);
			Item_setQuantity(item, amount, 1);
			if (temporary)
				Item_setTemporary(&item);
		} else
			break;
		if (TryToPlaceItem(PartyMembers[member], 0, 1)) {
			ZapDetachedItem(&item);
			if (++member == party)
				break;
		} else {
			placedMask |= 1 << member;
			count -= amount;
			unusedPlaced += amount;
		}
	}
	return placedMask;
}

/* Scrolls a window of the screen by whole 8-pixel text rows, as the video BIOS did in this mode,
 * blanking the rows it uncovers, then shows it for one display refresh. */
#define BIOS_SCROLL_MS 50

void BiosScrollWindow(int16_t down, int16_t lines, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	uint8_t *screen = ScreenPixels();
	int16_t left = (x1 >> 3) * 8;
	int16_t right = (x2 >> 3) * 8 + 7;
	int16_t top = (y1 >> 3) * 8;
	int16_t bottom = (y2 >> 3) * 8 + 7;
	int16_t shift = lines * 8;
	int16_t width, y;

	if (right > PLAT_SCREEN_WIDTH - 1)
		right = PLAT_SCREEN_WIDTH - 1;
	if (bottom > PLAT_SCREEN_HEIGHT - 1)
		bottom = PLAT_SCREEN_HEIGHT - 1;
	if (left > right || top > bottom)
		return;
	width = right - left + 1;
	if (shift == 0 || shift > bottom - top + 1)
		shift = bottom - top + 1;
	if (down == 1) {
		for (y = bottom; y >= top; y--) {
			if (y - shift >= top)
				memmove(screen + y * PLAT_SCREEN_WIDTH + left, screen + (y - shift) * PLAT_SCREEN_WIDTH + left, width);
			else
				memset(screen + y * PLAT_SCREEN_WIDTH + left, 0, width);
		}
	} else {
		for (y = top; y <= bottom; y++) {
			if (y + shift <= bottom)
				memmove(screen + y * PLAT_SCREEN_WIDTH + left, screen + (y + shift) * PLAT_SCREEN_WIDTH + left, width);
			else
				memset(screen + y * PLAT_SCREEN_WIDTH + left, 0, width);
		}
	}
	/* The BIOS scrolled mode 13h slowly, so each shaken position stayed up for a while; about
	 * one scroll's time on a 486. */
	uint32_t shown = plat_milliseconds();

	WaitForRetrace();
	while (plat_milliseconds() - shown < BIOS_SCROLL_MS)
		plat_yield();
}

void ShakeScreenForStep(void)
{
	HidePointer();
	StompCounter = (StompCounter + 1) % 6;
	if (StompCounter == 0) {
		BiosScrollWindow(0, 1, 0, 0, 320, 200);
		PlaySfx(8, 255, 64, 0);
	}
	ShowPointer();
}

void ShakeScreenForQuake(void)
{
	int16_t i;

	HidePointer();
	for (i = 0; i < 2; i++) {
		BiosScrollWindow(1, 1, 0, 0, 320, 200);
		BiosScrollWindow(0, 1, 0, 0, 320, 200);
	}
	PlaySfx(60, 255, 64, 0);
	ShowPointer();
	if (EarthquakeCount != 0)
		EarthquakeCount--;
}

void StartEarthquake(int16_t count)
{
	EarthquakeCount = count;
}

int16_t GetPartyMemberIndex(objref *member)
{
	return GetPartyIndex(*member);
}

objref GetOuterContainer(objref current, objref stop)
{
	objref previous;
	while (current.valid()) {
		if (current == stop)
			return current;
		previous = current;
		current = Item_getContainer(&current);
	}
	return previous;
}

uint8_t CanCarryWeight(objref container, objref item)
{
	if (NPCRef(container).isBody() && !HackMoverEnabled) {
		int16_t addedWeight = Item_getWeight(item) + DetermineWeightOfContents(item);
		int16_t capacity = (NPCRef(container).strength() * 2) * 10;
		int16_t contentsWeight = DetermineWeightOfContents(container);
		if (addedWeight + contentsWeight > capacity)
			return 0;
	}
	return 1;
}

uint8_t CanHoldBulk(objref container, objref item)
{
	if (!HackMoverEnabled) {
		int16_t addedBulk = GetItemTypeBulk(item.type());
		int16_t capacity = GetItemTypeBulk(container.type());
		int16_t contentsBulk = DetermineBulkOfContents(container);
		if (addedBulk + contentsBulk >= capacity && capacity != 0)
			return 0;
	}
	return 1;
}

void WearOffInvisibility(objref item)
{
	AreaSearch found;
	FindItemInContainer(&found, item, 1, 296, 255, 255);  /* ring of invisibility */
	if (found.current.valid()) {
		if (GenerateRandomIntegerInRange(10000) >= 5)
			return;
		Item_delete(&found.current);
	} else if (GenerateRandomIntegerInRange(1000) >= 5) {
		return;
	}
	Item_setQualityFlags(&item, Item_getQualityFlags(&item) & 0xfe);
}

extern "C" void ResetBogusGlobals(void)
{
	EarthquakeCount = 0;
	StompCounter = 0;
}
