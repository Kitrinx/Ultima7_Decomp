/* Serpent Isle SI.EXE, overlay segment 219 (file offsets 0x05c860 to 0x05d709, 3753 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: equip.c */
#include "objref.h"
#include "lowlevel.h"
#include "dosio.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "npcref.h"
#include "vooalloc.h"
#include "cast.h"
#include "itembuf.h"
#include "itemovr1.h"
#include "makemojo.h"
#include "usehook.h"
#include "wihh.h"
#include "u7npc.h"
#include "type.h"
#include "coord.h"
#include "voolook.h"
#include "equip.h"
#include "ready.h"
#include "script.h"
#include "actqueue.h"
#include "init.h"

#define MK_FP(seg, off) ((void _seg *)(seg) + (void near *)(off))
#define NUM_EQUIP_SLOTS 18

/* the address of an NPC's equipment slot in the equipment list */
#define EQUIP_SLOT(npc, slot) (EquipList + (npc) * (NUM_EQUIP_SLOTS * 2) + (slot) * 2)

#define ITEM(off) ((struct ItemRecord far *)MK_FP(ItemBufferSegment, (off)))

MemoryAddress EquipList;

extern unsigned char far Item_getQuantity(objref *ref);
extern char far Item_delete(objref *ref);
extern objref far Item_getContainer(objref *);
extern unsigned char far Item_moveIntoContainer(objref *ref, objref container);
extern void far Item_markEquipped(objref *);
extern void far Item_setWeaponReady(objref *);

inline char operator!=(const objref &a, const objref &b)
{
	return a.off != b.off;
}

#define CLASS_FLAGS(p) (ItemTypeClassFlags[gItemTypeInfo[(p)->typeFrame & 0x3ff].typeClass])
#define IS_NPC(p) ((unsigned char)((CLASS_FLAGS(p) & CLASS_NPC) != 0))
#define HAS_CONTENTS(p) ((unsigned char)(CLASS_FLAGS(p) & CLASS_CONTENTS))

extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern unsigned char far Item_move(objref *, Loc, Loc, int);
extern void far Item_move(objref *, unsigned char, int);

void QueueUnequipScript(ItemId item)
{
	unsigned char script[128];
	script[0] = 1;
	AppendScriptByte(script, 35);
	AppendScriptByte(script, 85);
	AppendScriptWord(script, 31003);
	ActionQueue.add(item.off, (char *)script);
}

unsigned char HasEquipUsecode(objref item)
{
	unsigned type;
	if (!item.valid())
		return 0;
	type = ITEM(item.off)->typeFrame & 0x3ff;
	switch (type) {
	case 209: case 296: case 336: case 338: case 595: case 701:
	case 806: case 990: case 996: case 1001: case 1013:
		return 1;
	}
	return 0;
}

void InitEquipment(void)
{
	int slots[NUM_EQUIP_SLOTS];
	int i;
	EquipList.address = AllocateVoodooMemory(&VoodooXmsBlock,
		(unsigned)((NpcRecordCount + ExtraNpcRecordCount) * sizeof slots));
	if ((unsigned char)(EquipList.address == 0)) ReportError(0xe901);
	for (i = 0; i < NUM_EQUIP_SLOTS; i++) slots[i] = 0;
	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++)
		CopyFarToLinear(EquipList + i * sizeof slots, slots, (long)sizeof slots);
}

unsigned char EquipItem(ItemId item, ItemId wearer, unsigned char slot, unsigned char combine)
{
	objref ref, unusedOwner;
	unsigned char occupied = 1;
	int npcNum;
	ref.off = item.off;
	unusedOwner.off = wearer.off;
	npcNum = Item_getNpcNumber(&NPCRef(wearer.off));
	if (npcNum != -1) {
		/* Slot 20 takes both hands. */
		if (slot == 20)
			occupied = PeekWord(EQUIP_SLOT(npcNum, 1)) != 0 || PeekWord(EQUIP_SLOT(npcNum, 0)) != 0;
		else if (slot < NUM_EQUIP_SLOTS)
			occupied = PeekWord(EQUIP_SLOT(npcNum, slot)) != 0;
		else if (slot == NUM_EQUIP_SLOTS)
			HaltWithMessage(__FILE__, 214, "tried to clear out the EquipList[NUM_EQUIP_SLOTS]!");
		if (occupied) {
			if (combine) {
				objref existing = GetItemInSlot(wearer, slot);
				if (CanStackWith(&existing, item.off)) {
					Item_setQuantity(existing, Item_getQuantity(&ref) + Item_getQuantity(&existing), 0);
					Item_delete(&ref);
					return 1;
				}
			}
			return 0;
		} else {
			if (Item_getContainer(&ref) != wearer.off || (char)(GetItemKind(&ref) == 7)) {
				Item_moveIntoContainer(&ref, wearer.off);
			}
			if (slot == 20) {
				PokeWord(EQUIP_SLOT(npcNum, 1), item.off);
				PokeWord(EQUIP_SLOT(npcNum, 0), item.off);
			} else
				PokeWord(EQUIP_SLOT(npcNum, slot), item.off);
			Item_markEquipped(&ref);
			if (slot == 1 || slot == 20) {
				Item_setWeaponReady(&NPCRef(wearer.off));
			}
			RunEquipUsecode(ref);
			return 1;
		}
	}
	return 0;
}

void RunEquipUsecode(objref ref)
{
	if (HasEquipUsecode(ref)) {
		if ((((ITEM(ref.off)->typeFrame & 0x3ff) == 296 &&
			ref.frame() == 0) ||
			((ITEM(ref.off)->typeFrame & 0x3ff) == 1013 &&
			ref.frame() == 0)) && !(unsigned char)UsableRunning)
			RunUsable(5, ref.off, -1);
		else if ((ITEM(ref.off)->typeFrame & 0x3ff) != 806) {
			unsigned char script[128];
			script[0] = 1;
			AppendScriptByte(script, 35);
			AppendScriptByte(script, 85);
			AppendScriptWord(script, 31002);
			ActionQueue.add(ref.off, (char *)script);
		}
	}
}

unsigned char CanEquipInSlot(objref item, ItemId wearer, unsigned char slot, unsigned char combine)
{
	unsigned char needed;
	unsigned type;
	type = ITEM(item.off)->typeFrame & 0x3ff;
	needed = ReadyRecords.get(ReadyLookup.get(type))->slot;
	if (combine) {
		objref existing;
		existing = objref(GetItemInSlot(wearer, slot));
		if (existing.valid()) return CanStackWith(&existing, item);
	}
	if ((char)gItemTypeInfo[type].light && slot != 1 && slot != 0 &&
		type != 551 && type != 553 && type != 549 && type != 926 && type != 1013) return 0;
	if ((ITEM(item.off)->typeFrame & 0x3ff) == 704 && (slot == 1 || slot == 0)) return 0;
	if ((needed == 8 || needed == 5 || needed == 6) && (slot == 1 || slot == 0)) return 0;
	if ((slot == 1 || slot == 0) && needed != 20) needed = slot;
	if (slot == 11) {
		switch (ITEM(item.off)->typeFrame & 0x3ff) {
		case 231: case 474: case 508: case 520: case 535: case 547: case 549: case 551: case 552:
		case 563: case 564: case 567: case 584: case 590: case 593: case 594: case 595: case 596:
		case 598: case 599: case 604: case 605: case 608: case 622: case 623: case 629: case 630:
		case 636: case 637: case 659: case 698: case 710: case 771: case 792: case 802: case 926:
		case 990: case 994: case 996:
			return objref(GetItemInSlot(wearer, 11)).valid() ? 0 : 1;
		}
		return 0;
	} else if (slot == 15) {
		switch (ITEM(item.off)->typeFrame & 0x3ff) {
		case 583: case 801:
			return objref(GetItemInSlot(wearer, 15)).valid() ? 0 : 1;
		}
		return 0;
	} else if (slot == 16) {
		switch (ITEM(item.off)->typeFrame & 0x3ff) {
		case 490: case 543: case 545: case 572: case 578: case 585: case 586: case 609: case 663: case 729:
			return objref(GetItemInSlot(wearer, 16)).valid() ? 0 : 1;
		}
		return 0;
	} else if (slot == 17) {
		switch (ITEM(item.off)->typeFrame & 0x3ff) {
		case 241: case 553: case 557: case 583: case 589: case 592: case 597: case 600: case 601:
		case 602: case 603: case 606: case 618: case 620: case 624: case 625: case 626: case 640:
		case 662: case 711: case 806: case 942:
			return objref(GetItemInSlot(wearer, 17)).valid() ? 0 : 1;
		}
		return 0;
	} else {
		switch (needed) {
		case 0: case 2: case 3: case 4: case 5: case 6: case 7:
		case 9: case 10: case 12: case 13: case 14: case 15:
		{
			if (needed == slot) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		}
		case 1:
			if (slot == 1 || slot == 0) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		case 20:
			if (objref(GetItemInSlot(wearer, 1)).valid() != 0 ||
				objref(GetItemInSlot(wearer, 0)).valid() != 0) return 0;
			if (slot == 1 || slot == 0 || slot == 20) return 1;
			return 0;
		case 8:
			if (slot == 8 || slot == 7) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		}
	}
	return 0;
}

void UnequipItem(ItemId id)
{
	objref item = id.off;
	objref owner, container;
	if (!item.valid()) return;
	if (!(unsigned char)(GetItemKind(&item) >= 6)) return;
	owner = Item_getContainer(&item);
	Item_moveIntoContainer(&item, owner);
	if (!IS_NPC(ITEM(owner.off))) return;
	if (!(unsigned char)((GetNpcBufferForIbo(&NPCRef(owner))->status & NPC_IN_PARTY) != 0)) return;
	unsigned char i = 0;
	unsigned char preferredSlots[3] = {15, 11, 17};
	for (; i < 3; i++) {
		switch (i) {
		case 0:
			container = objref(GetItemInSlot(owner, preferredSlots[i]));
			if (container.valid() && HAS_CONTENTS(ITEM(container.off))) {
				Item_moveIntoContainer(&item, container);
				return;
			}
			break;
		case 1: case 2:
			container = objref(GetItemInSlot(owner, preferredSlots[i]));
			if (container.valid() && HAS_CONTENTS(ITEM(container.off))) {
				Item_moveIntoContainer(&item, container);
				return;
			}
			if (CanEquipInSlot(item, NPCRef(owner), preferredSlots[i], 0)) {
				EquipItem(item, NPCRef(owner), preferredSlots[i], 0);
				return;
			}
			break;
		}
	}
	Item_move(&item, Item_getX(owner), Item_getY(owner), Item_getZ(&owner));
}
