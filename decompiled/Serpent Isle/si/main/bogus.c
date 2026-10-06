/* Serpent Isle SI.EXE, overlay segment 212 (file offsets 0x052e50 to 0x05408d, 4669 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: bogus.c */
#include <stdio.h>
#include <dos.h>
#include "lowlevel.h"
#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "item.h"
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
#include "crimeov1.h"
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
#include "text.h"
#include "script.h"
#include "actqueue.h"
#include "keyring.h"

extern objref far Item_getContainer(objref *);
extern objref far GetContainedItem(objref *);

struct Rect : Point {
	int x1, y1;
	Rect(int a, int b, int c, int d) : Point(a, b) { x1 = c; y1 = d; }
	int contains(int px, int py) { return (px >= x) & (px <= x1) & (py >= y) & (py <= y1); }
};

inline unsigned char GetEquipmentSlot(unsigned shape) { return ReadyRecords.get(ReadyLookup.get(shape))->slot; }

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far Item_getQuantity(objref *ref);
extern unsigned char far Item_isOkayToTake(objref *);
extern void far Item_setOkayToTake(objref *);
extern int far GetItemBeingDragged(objref *ref);
extern unsigned char far ZapDetachedItem(objref *ref);
extern unsigned char far CreateItem(objref *, TypeFrame);
extern void far Item_setFrame(objref *, int);
extern void far Item_setQuality(objref *, char);
extern void far Item_setTemporary(objref *);
extern void far Item_clearTemporary(objref *);
extern char far Item_delete(objref *ref);
extern unsigned char far Item_getQualityFlags(objref *);
extern void far Item_setQualityFlags(objref *, unsigned char);
extern "C" void far PlaySfx(unsigned char number, unsigned volume, int pan);

int EarthquakeCount;
unsigned char StompCounter = 0;

void far EmptyRoutine()
{
}

void far PickWorldCoords(int x, int y, Coord *wx, Coord *wy, int *wz, char flag)
{
	objref none = 0;
	PickWorldCoordsOnItem(x, y, wx, wy, wz, &none, flag);
}

void far PickWorldCoordsOnItem(int x, int y, Coord *wx, Coord *wy, int *wz, objref *target, char flag)
{
	if (!target->valid())
		FindItemAtScreenPoint(target, x, y, &MainWorldView, &gShapeManager, 1);
	if (target->valid()) {
		int rise = (unsigned char)(target->z() + gItemTypeInfo[target->type()].height) * 4;
		int w = GetFootprintX(*(TypeFrame far *)&target->ptr()->typeFrame);
		int h = GetFootprintY(*(TypeFrame far *)&target->ptr()->typeFrame);
		int tx, ty;
		WorldCoordsToScreen(Item_getX(*target), Item_getY(*target), &tx, &ty);
		int x0 = tx - w * 8 - rise - 8;
		int y0 = ty - h * 8 - rise - 8;
		int x1 = tx - rise;
		int y1 = ty - rise;
		Rect r(x0, y0, x1, y1);
		if (!r.contains(x, y)) {
			if (x < x0)
				x = x0;
			else if (x > x1)
				x = x1;
			if (y < y0)
				y = y0;
			else if (*wy > y1)     /* tests *wy, as shipped */
				y = y1;
		}
		ScreenToWorldCoords(x + rise, y + rise, &wx->value, &wy->value, flag);
		*wz = (unsigned char)(target->z() + gItemTypeInfo[target->type()].height);
	} else {
		ScreenToWorldCoords(x, y, &wx->value, &wy->value, flag);
		*wz = 0;
	}
}

void far CheatCastSpell()
{
	int spell;
	DebugPrintfAtCoords(1, 1, GetGameText(3, 214));
	scanf("%d", &spell);
	TryToCastSpell(AvatarRef, spell, 0, 0, 1);
}

int far CountHeldItems(char allParty, objref container, int type, int quality, int frame)
{
	int quantity = 0;
	AreaSearch found;
	if (allParty) {
		for (int i = PartySize - 1; i >= 0; i--) {
			if (FindItemInContainer(&found, PartyMembers[i], 0, type, quality, frame)) {
				do {
					if (Item_getContainer(&found.current).type() != 522) {
						if ((int)Item_getQuantity(&found.current) == 0)
							quantity++;
						else
							quantity += Item_getQuantity(&found.current);
					}
				} while (FindItem(&found));
			}
		}
	} else {
		if (FindItemInContainer(&found, container, 0, type, quality, frame)) {
			do {
				if (Item_getContainer(&found.current).type() != 522) {
					if ((int)Item_getQuantity(&found.current) == 0)
						quantity++;
					else
						quantity += Item_getQuantity(&found.current);
				}
			} while (FindItem(&found));
		}
	}
	return quantity;
}

void far MarkItemOkayToTake(objref item)
{
	AreaSearch found;
	if (!Item_isOkayToTake(&item)) {
		if (!HackMoverEnabled && !TheftReported) {
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

unsigned char far AddKeyToKeyring(NPCRef owner, objref key)
{
	AreaSearch found;
	unsigned char script[128];
	if (FindItemInContainer(&found, owner, 0, 485, 255, 255)) {
		if (KeyRing.acceptsKey(key.frame(), Item_getQuality(&key))) {
			script[0] = 1;
			AppendScriptByte(script, SCRIPT_NO_HALT);
			AppendScriptByte(script, SCRIPT_USECODE);
			AppendScriptWord(script, 1675);
			ActionQueue.add(found.current.off, (char *) script);
		} else {
			KeyRing.add(Item_getQuality(&key));
			if (!ZapDetachedItem(&key))
				AssertFail(__FILE__, 311);
			return 1;
		}
	}
	return 0;
}

int far TryToPlaceItem(objref target, unsigned char combine, unsigned char mark)
{
	objref dragged, occupied, container, npc;
	unsigned char inBody = 0;
	int slot = -1;
	int error = 0;
	GetItemBeingDragged(&dragged);
	if (target.isBody()) {
		npc = target;
		container = 0;
		if ((unsigned char)((GetNpcBufferForIbo(&npc)->status & NPC_IN_PARTY) != 0) && mark)
			Item_setOkayToTake(&dragged);
		Item_clearTemporary(&dragged);
		if (dragged.type() == 641 && AddKeyToKeyring((NPCRef &) npc, dragged))
			return 0;
	} else {
		container = target;
		npc = 0;
	}
	if (npc.valid()) {
		int state = 1;
		while (state) {
			switch (state) {
			case 1:
				switch (slot = GetEquipmentSlot(dragged.type())) {
				case 0:
				case 1:
					if (dragged.type() == 490 || dragged.type() == 572 || dragged.type() == 578 ||
						dragged.type() == 585 || dragged.type() == 586 || dragged.type() == 609 ||
						dragged.type() == 663 || dragged.type() == 543 || dragged.type() == 729) {
						slot = 0;
						state = 2;
					} else {
						slot = 1;
						state = 3;
					}
					break;
				case 20:
					state = 4;
					break;
				case 8:
					slot = 8;
					state = 9;
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
				slot = 0;
				state = 4;
				break;
			case 4:
				slot = 11;
				state = 5;
				break;
			case 5:
				slot = 15;
				state = 6;
				break;
			case 6:
				slot = 16;
				state = 7;
				break;
			case 7:
				slot = 17;
				state = 8;
				break;
			case 8:
				state = 0;
				break;
			case 9:
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
					int type = occupied.type();
					if (type != 522 && type != 914 && type != 762 && type != 892 && type != 402 &&
						type != 778 && type != 414 && type != 400 &&
						(type != 555 || (type == 555 && dragged.type() == 559)) && CanHoldBulk(occupied, dragged)) {
						if (!(unsigned char)gItemTypeInfo[dragged.type()].light || dragged.type() == 553
							|| dragged.type() == 551 || dragged.type() == 926 || dragged.type() == 549 ||
							dragged.type() == 1013 || HackMoverEnabled) {
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
			int type = container.type();
			if (type == 522 || type == 914 || (type == 555 && dragged.type() != 559) ||
				((type == 400 || type == 414 || type == 778 || type == 402 || type == 892 || type == 762) &&
				!HackMoverEnabled)) {
				error = 3;
			} else if (!CanHoldBulk(container, dragged)) {
				error = 4;
			} else if ((unsigned char)gItemTypeInfo[dragged.type()].light && !HackMoverEnabled) {
				int type = dragged.type();
				if (type != 553 && type != 551 && type != 926 && type != 549 && type != 1013)
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
				EquipItem(dragged, target, slot, combine);
			}
			if (inBody) {
				MarkItemOkayToTake(dragged);
				Item_clearTemporary(&dragged);
			}
			return 0;
		}
		error = 3;
	} else {
		error = 3;
	}
	return error;
}

unsigned far GiveItemsToParty(int *count, int type, int quality, int frame, int temporary)
{
	objref item;
	int member = 0;
	unsigned placedMask = 0;
	int party;
	TypeFrame typeFrame = 0;
	int unusedPlaced;
	int amount;
	typeFrame.setType(type);
	typeFrame.setFrame(ClampShapeFrame(type, frame));
	party = PartySize;
	amount = 1;
	unusedPlaced = 0;
	while (*count) {
		if (gItemTypeInfo[typeFrame.type()].typeClass == TYPE_CLASS_QUANTITY)
			amount = *count > 100 ? 100 : *count;
		if (CreateItem(&item, typeFrame)) {
			Item_setFrame(&item, frame);
			Item_setQuality(&item, quality);
			Item_setQuantity(item, amount, 1);
			if (temporary)
				Item_setTemporary(&item);
		} else
			break;
		Item_setOkayToTake(&item);
		Item_clearTemporary(&item);
		if (TryToPlaceItem(PartyMembers[member], 0, 1)) {
			ZapDetachedItem(&item);
			++member;
			unusedPlaced = 0;
			if (member == party)
				break;
		} else {
			placedMask |= 1 << member;
			*count -= amount;
			unusedPlaced += amount;
		}
	}
	return placedMask;
}

void far BiosScrollWindow(int down, int lines, int x1, int y1, int x2, int y2)
{
	union REGS regs;
	regs.h.ah = down == 1 ? 7 : 6;
	regs.h.al = lines;
	regs.h.ch = y1 >> 3;
	regs.h.cl = x1 >> 3;
	regs.h.dh = y2 >> 3;
	regs.h.dl = x2 >> 3;
	regs.h.bh = 0;
	int86(0x10, &regs, &regs);
}

void far ShakeScreenForStep(void)
{
	HidePointer();
	StompCounter = (StompCounter + 1) % 6;
	if (StompCounter == 0) {
		BiosScrollWindow(0, 1, 0, 0, 320, 200);
		PlaySfx(8, 255, 64);
	}
	ShowPointer();
}

void far ShakeScreenForQuake(void)
{
	int i;

	HidePointer();
	for (i = 0; i < 2; i++) {
		BiosScrollWindow(1, 1, 0, 0, 320, 200);
		BiosScrollWindow(0, 1, 0, 0, 320, 200);
	}
	PlaySfx(62, 255, 64);
	ShowPointer();
	if (EarthquakeCount != 0)
		EarthquakeCount--;
}

void far StartEarthquake(int count)
{
	EarthquakeCount = count;
}

int far GetPartyMemberIndex(objref *member)
{
	return GetPartyIndex(*member);
}

objref far GetOuterContainer(objref current, objref stop)
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

unsigned char far CanCarryWeight(objref container, objref item)
{
	if (NPCRef(container).isBody() && !HackMoverEnabled) {
		int addedWeight = Item_getWeight(item) + DetermineWeightOfContents(item);
		int capacity = (NPCRef(container).strength() * 2) * 10;
		int contentsWeight = DetermineWeightOfContents(container);
		if (addedWeight + contentsWeight > capacity)
			return 0;
	}
	return 1;
}

unsigned char far CanHoldBulk(objref container, objref item)
{
	if (!HackMoverEnabled) {
		int addedBulk = GetItemTypeBulk(item.type());
		int capacity = GetItemTypeBulk(container.type());
		int contentsBulk = DetermineBulkOfContents(container);
		if (addedBulk + contentsBulk >= capacity && capacity != 0)
			return 0;
	}
	return 1;
}

void far WearOffInvisibility(objref item)
{
	AreaSearch found;
	FindItemInContainer(&found, item, 1, 296, 255, 0);  /* ring of invisibility */
	if (found.current.valid()) {
		if (GenerateRandomIntegerInRange(10000) >= 5)
			return;
		Item_delete(&found.current);
	} else if (GenerateRandomIntegerInRange(1000) >= 5) {
		return;
	}
	Item_setQualityFlags(&item, Item_getQualityFlags(&item) & 0xfe);
}
