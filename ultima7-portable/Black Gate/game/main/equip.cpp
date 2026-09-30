/* Black Gate U7.EXE, overlay segment 230 (file offsets 0x06b010 to 0x06bb9e, 2958 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
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

#define NUM_EQUIP_SLOTS 12

/* the address of an NPC's equipment slot in the equipment list */
#define EQUIP_SLOT(npc, slot) (EquipList + (npc) * 24 + (slot) * 2)

#define ITEM(off) ((struct ItemRecord *)ItemAt((off)))

MemoryAddress EquipList;

extern uint8_t Item_getQuantity(objref *ref);
extern int8_t Item_delete(objref *ref);
extern objref Item_getContainer(objref *);
extern uint8_t Item_moveIntoContainer(objref *ref, objref container);
extern void Item_markEquipped(objref *);
extern void Item_setWeaponReady(objref *);

#define CLASS_FLAGS(p) (ItemTypeClassFlags[gItemTypeInfo[(p)->typeFrame & 0x3ff].typeClass])
#define IS_NPC(p) ((uint8_t)((CLASS_FLAGS(p) & CLASS_NPC) != 0))
#define HAS_CONTENTS(p) ((uint8_t)(CLASS_FLAGS(p) & CLASS_CONTENTS))

extern Coord Item_getX(objref &);
extern Coord Item_getY(objref &);
extern uint8_t Item_move(objref *, Loc, Loc, int16_t);
extern void Item_move(objref *, uint8_t, int16_t);

uint8_t HasEquipUsecode(objref item)
{
	uint16_t type;
	if (!item.valid())
		return 0;
	type = ITEM(item.off)->typeFrame & 0x3ff;
	switch (type) {
	case 296:   /* ring of invisibility */
	case 298:   /* ring of regeneration */
	case 336:   /* light source */
	case 338:   /* lit light source */
	case 595:   /* torch */
	case 701:   /* lit torch */
		return 1;
	}
	return 0;
}

void InitEquipment(void)
{
	int16_t slots[NUM_EQUIP_SLOTS];
	int16_t i;
	EquipList.address = AllocateVoodooMemory(&VoodooXmsBlock,
		(uint16_t)((NpcRecordCount + ExtraNpcRecordCount) * sizeof slots));
	if ((uint8_t)(EquipList.address == 0)) ReportError(0xe901);
	for (i = 0; i < NUM_EQUIP_SLOTS; i++) slots[i] = 0;
	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++)
		CopyFarToLinear(EquipList + (int32_t)(i * sizeof slots), slots, (int32_t)sizeof slots);
}

uint8_t EquipItem(ItemId item, ItemId wearer, uint8_t slot, uint8_t combine)
{
	objref ref, unusedOwner;
	uint8_t occupied = 1;
	int16_t npcNum;
	ref.off = item.off;
	unusedOwner.off = wearer.off;
	npcNum = Item_getNpcNumber(&NPCRef(wearer.off));
	if (npcNum != -1) {
		/* slot 20 takes both hands, 21 both of slots 6 and 7 */
		if (slot == 20)
			occupied = PeekWord(EQUIP_SLOT(npcNum, 1)) != 0 || PeekWord(EQUIP_SLOT(npcNum, 2)) != 0;
		else if (slot == 21)
			occupied = PeekWord(EQUIP_SLOT(npcNum, 6)) != 0 || PeekWord(EQUIP_SLOT(npcNum, 7)) != 0;
		else if (slot <= 12)
			occupied = PeekWord(EQUIP_SLOT(npcNum, slot)) != 0;
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
			if (Item_getContainer(&ref) != wearer.off || (int8_t)(GetItemKind(&ref) == 7)) {
				Item_moveIntoContainer(&ref, wearer.off);
			}
			if (slot == 20) {
				PokeWord(EQUIP_SLOT(npcNum, 1), item.off);
				PokeWord(EQUIP_SLOT(npcNum, 2), item.off);
			} else if (slot == 21) {
				PokeWord(EQUIP_SLOT(npcNum, 6), item.off);
				PokeWord(EQUIP_SLOT(npcNum, 7), item.off);
			} else
				PokeWord(EQUIP_SLOT(npcNum, slot), item.off);
			Item_markEquipped(&ref);
			if (slot == 1 || slot == 20) {
				Item_setWeaponReady(&NPCRef(wearer.off));
			}
			if (HasEquipUsecode(ref)) RunUsable(5, ref.off, -1);
			return 1;
		}
	}
	return 0;
}

uint8_t CanEquipInSlot(objref item, ItemId wearer, uint8_t slot, uint8_t combine)
{
	uint8_t needed;
	uint16_t type;
	type = ITEM(item.off)->typeFrame & 0x3ff;
	needed = ReadyRecords.get(ReadyLookup.get(type))->slot;
	if (combine) {
		objref existing;
		existing = objref(GetItemInSlot(wearer, slot));
		if (existing.valid()) return CanStackWith(&existing, item);
	}
	/* 551 fire sword, 553 firedoom staff */
	if ((int8_t)gItemTypeInfo[type].light && slot != 1 && slot != 2 && type != 551 && type != 553) return 0;
	if ((ITEM(item.off)->typeFrame & 0x3ff) == 704 && (slot == 1 || slot == 2)) return 0;   /* powder keg */
	if ((slot == 1 || slot == 2 || slot == 0) && needed != 20) needed = slot;
	if (slot == 3) {
		if (needed == 1 || needed == 2 || needed == 20)
			return objref(GetItemInSlot(wearer, 3)).valid() ? 0 : 1;
		return 0;
	} else {
		switch (needed) {
		case 0: case 2: case 4: case 5: case 7: case 8: case 9: case 10: case 11:
			if (needed == slot) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		case 1:
			if (slot == 1 || slot == 2) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		case 20:
			if (objref(GetItemInSlot(wearer, 1)).valid() != 0 ||
				objref(GetItemInSlot(wearer, 2)).valid() != 0) return 0;
			if (slot == 1 || slot == 2 || slot == 20) return 1;
			return 0;
		case 6:
			if (slot == 6 || slot == 7) return objref(GetItemInSlot(wearer, slot)).valid() ? 0 : 1;
			return 0;
		case 21:
			if (objref(GetItemInSlot(wearer, 6)).valid() != 0 ||
				objref(GetItemInSlot(wearer, 7)).valid() != 0) return 0;
			if (slot == 6 || slot == 7 || slot == 21) return 1;
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
	if (!(uint8_t)(GetItemKind(&item) >= 6)) return;
	owner = Item_getContainer(&item);
	Item_moveIntoContainer(&item, owner);
	if (!IS_NPC(ITEM(owner.off))) return;
	if (!(uint8_t)((GetNpcBufferForIbo(&NPCRef(owner))->status & NPC_IN_PARTY) != 0)) return;
	container = objref(GetItemInSlot(owner, 0));
	if (!container.valid() || !HAS_CONTENTS(ITEM(container.off))) {
		container = objref(GetItemInSlot(owner, 3));
		if (container.valid()) {
			if (!HAS_CONTENTS(ITEM(container.off)))
				Item_move(&item, Item_getX(owner), Item_getY(owner), Item_getZ(&owner));
			else
				Item_moveIntoContainer(&item, container);
		} else {
			if (CanEquipInSlot(item, NPCRef(owner), 3, 0))
				EquipItem(item, NPCRef(owner), 3, 0);
			else
				Item_move(&item, Item_getX(owner), Item_getY(owner), Item_getZ(&owner));
		}
	} else
		Item_moveIntoContainer(&item, container);
}

extern "C" void ResetEquipGlobals(void)
{
	memset(&EquipList, 0, sizeof(EquipList));
}
