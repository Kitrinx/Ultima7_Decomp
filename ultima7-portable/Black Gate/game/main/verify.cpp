/* Black Gate U7.EXE, resident segment 65 (file offsets 0x025dd9 to 0x02691e, 2885 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include "objref.h"
#include "itemrec.h"
#include "memapi.h"
#include "npcref.h"
#include "wihh.h"
#include "u7npc.h"
#include "type.h"

/* The item buffer holds 6668 records of 8 bytes; the first 7 are reserved. */
#define ITEM_RECORDS    6668
#define FIRST_RECORD    7

/* What ItemCheck found in each record. */
#define MARK_FREE       0x01    /* on the free list */
#define MARK_ITEM       0x02    /* an item reached from a list */
#define MARK_NPC        0x04    /* an NPC */
#define MARK_EXTRA      0x08    /* an item's extended record */
#define MARK_ORPHAN     0x10    /* reached by nothing, yet holding data */
#define MARK_LOST       0x20    /* reached by nothing and empty */
#define MARK_DETACHED   0x40    /* on the detached list */

/* Walks every list of the item buffer, marking each record in map, to find records that are bad,
 * doubly used or lost. */
struct ItemCheck {
	uint8_t *map;
	int8_t bad, lost;
	int8_t isBadRef(objref r);
	void clear();
	void markFreeList();
	int8_t isRecordUsed(objref r);
	int8_t markItem(objref r);
	void markContents(objref r);
	void markChunkLists();
	void markNpcs();
	void markDetached();
	void findLostRecords();
	void run();
};

/* Counts the records the item buffer's lists use; with the free ones they must fill it. */
struct ItemCounter {
	int8_t bad, lost;
	int16_t count;
	void clear();
	void countFreeList();
	int8_t countItem(objref itemRef, uint8_t includeNpcs);
	void countContents(objref container);
	void countChunkLists();
	void countNpcs();
	void countDetached();
	void run();
};

extern int16_t ItemFreeList;
extern objref DetachedItems;
extern int16_t ChunkItemLists[4][16][16];
extern int16_t ItemFreeCount;
extern objref GetContainedItem(objref *);
extern int16_t Item_getExtraRecord(objref *);

#define ITEM(off) ((ItemRecord *)ItemAt((off)))
#define TYPE_CLASS(p) (gItemTypeInfo[(p)->typeFrame & 0x3ff].typeClass)
#define CLASS_FLAGS(p) (ItemTypeClassFlags[TYPE_CLASS(p)])
#define KIND(p) ((uint8_t)((uint8_t)CLASS_FLAGS(p) & CLASS_RECORDS))
#define IS_NPC(p) ((uint8_t)((CLASS_FLAGS(p) & CLASS_NPC) != 0))

int8_t ItemCheck::isBadRef(objref r)
{
	if ((uint16_t)r.off >= ITEM_RECORDS * 8U || (r.valid() && (uint16_t)r.off < FIRST_RECORD * 8)) {
		bad = 1;
		return 1;
	}
	if (r.off & 7) {
		bad = 1;
		return 1;
	}
	return 0;
}

void ItemCheck::clear()
{
	int16_t i;
	for (i = FIRST_RECORD; i < ITEM_RECORDS; i++)
		map[i] = 0;
	bad = lost = 0;
}

void ItemCheck::markFreeList()
{
	int16_t *unusedPrev;
	objref cur = ItemFreeList;
	int16_t count;
	uint16_t i;
	unusedPrev = &ItemFreeList;
	count = 0;
	while (cur.valid()) {
		if (isBadRef(cur) != 0)
			break;
		i = (uint16_t)cur.off >> 3;
		if (map[i] & MARK_FREE) {
			bad = 1;
			break;
		}
		map[i] |= MARK_FREE;
		if (map[i] & ~MARK_FREE) {
			bad = 1;
			break;
		}
		count++;
		unusedPrev = &ITEM(cur.off)->next;
		cur = cur.next();
	}
	if (count != ItemFreeCount)
		bad = lost = 1;
}

int8_t ItemCheck::isRecordUsed(objref r)
{
	uint16_t i;
	if (isBadRef(r))
		return 1;
	i = (uint16_t)r.off >> 3;
	if (map[i] != 0) {
		bad = 1;
		return 1;
	}
	return 0;
}

int8_t ItemCheck::markItem(objref r)
{
	int16_t extra;
	uint16_t i;
	int16_t kind;
	i = (uint16_t)r.off >> 3;
	if (isBadRef(r) != 0)
		return 1;
	if ((map[i] & MARK_NPC) == 0) {
		if (isRecordUsed(r) != 0)
			return 1;
		map[i] |= MARK_ITEM;
		kind = KIND(ITEM(r.off));
		if (kind > 0) {
			extra = ITEM(r.off)->data.extra;
			if (isRecordUsed(extra) != 0) {
				map[i] &= ~MARK_ITEM;
				return 1;
			}
			i = (uint16_t)extra >> 3;
			map[i] |= MARK_EXTRA;
			if (kind == 3) {
				extra = ITEM(extra)->data.extra;
				if (isRecordUsed(extra) != 0) {
					map[(uint16_t)r.off >> 3] &= ~MARK_ITEM;
					map[i] &= ~MARK_EXTRA;
					return 1;
				}
				i = (uint16_t)extra >> 3;
				map[i] |= MARK_EXTRA;
			}
		}
	}
	return 0;
}

void ItemCheck::markContents(objref r)
{
	objref cur;
	int16_t *unusedPrev;
	unusedPrev = (int16_t *)ItemAt(Item_getExtraRecord((objref *)&r));
	cur = GetContainedItem((objref *)&r);
	while (cur.valid()) {
		if (markItem(cur) != 0)
			break;
		if (GetContainedItem((objref *)&cur).valid()) {
			markContents(cur);
		}
		unusedPrev = (int16_t *)ItemAt(cur.off);
		cur = cur.next();
	}
}

void ItemCheck::markChunkLists()
{
	int16_t region, y, x;
	objref cur;
	objref *prev;
	for (region = 0; region < 4; region++) {
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 16; x++) {
				prev = (objref *)&ChunkItemLists[region][y][x];
				cur.off = prev->off;
				if (isBadRef(cur) != 0)
					continue;
				while (cur.valid()) {
					if (markItem(cur) != 0)
						break;
					if (!IS_NPC(ITEM(cur.off))) {
						markContents(cur);
					}
					prev = (objref *)ItemAt(cur.off);
					cur = cur.next();
				}
			}
		}
	}
}

void ItemCheck::markNpcs()
{
	int16_t i, idx;
	uint16_t npc;
	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if ((int8_t)(npc != 0)) {
			if (isBadRef(npc) == 0) {
				if (markItem(npc) == 0) {
					idx = npc >> 3;
					map[idx] |= MARK_NPC;
					markContents(npc);
				}
			}
		}
	}
}

void ItemCheck::markDetached()
{
	int16_t *unusedPrev;
	objref cur = DetachedItems;
	int16_t i;
	if (isBadRef(cur) != 0)
		return;
	unusedPrev = &DetachedItems.off;
	while (cur.valid()) {
		if (markItem(cur) != 0)
			return;
		i = (uint16_t)cur.off >> 3;
		map[i] |= MARK_DETACHED;
		if (!IS_NPC(ITEM(cur.off))) {
			markContents(cur);
		}
		unusedPrev = (int16_t *)ItemAt(cur.off);
		cur = cur.next();
	}
}

void ItemCheck::findLostRecords()
{
	int16_t nonempty, empty;
	objref current;
	int16_t *data;
	int16_t i;
	nonempty = empty = 0;
	current.off = FIRST_RECORD * 8;
	for (i = FIRST_RECORD; i < ITEM_RECORDS; i++) {
		if (map[i] == 0) {
			data = (int16_t *)ItemAt(current.off);
			data++;
			if (data[0] != 0 || data[1] != 0 || data[2] != 0) {
				nonempty++;
				bad = 1;
				map[i] |= MARK_ORPHAN;
			} else {
				empty++;
				bad = lost = 1;
				map[i] |= MARK_LOST;
			}
		}
		current.off += 8;
	}
}

void ItemCheck::run()
{
	clear();
	markFreeList();
	markDetached();
	markNpcs();
	markChunkLists();
	findLostRecords();
}

void ItemCounter::clear()
{
	bad = lost = 0;
	count = 0;
}

void ItemCounter::countFreeList()
{
	objref cur = ItemFreeList;
	int16_t count;  /* uninitialised: the loop counts the member */
	while (cur.valid()) {
		this->count++;
		cur = cur.next();
	}
	if (count != ItemFreeCount)
		lost = 1;
}

uint8_t ItemCheckStub(void)
{
	return 0;
}

int8_t ItemCounter::countItem(objref itemRef, uint8_t includeNpcs)
{
	int16_t kind;
	int16_t extra;

	if (!includeNpcs && IS_NPC(ITEM(itemRef.off)))
		return 0;
	count++;
	kind = KIND(ITEM(itemRef.off));
	if (kind > 0) {
		extra = ITEM(itemRef.off)->data.extra;
		count++;
		if (kind == 3) {
			extra = ITEM(extra)->data.extra;
			count++;
		}
	}
	return 0;
}

void ItemCounter::countContents(objref container)
{
	objref cur;

	cur = GetContainedItem((objref *)&container);
	while (cur.valid()) {
		if (countItem(cur, 0) != 0)
			break;
		if (GetContainedItem((objref *)&cur).valid()) {
			countContents(cur);
		}
		cur = cur.next();
	}
}

void ItemCounter::countChunkLists()
{
	int16_t region, y, x;
	objref cur;
	objref *list;

	for (region = 0; region < 4; region++) {
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 16; x++) {
				list = (objref *)&ChunkItemLists[region][y][x];
				cur.off = list->off;
				while (cur.valid()) {
					if (countItem(cur, 0) != 0)
						break;
					if (!IS_NPC(ITEM(cur.off))) {
						countContents(cur);
					}
					cur = cur.next();
				}
			}
		}
	}
}

void ItemCounter::countNpcs()
{
	int16_t npc;
	int16_t i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if ((int8_t) (npc != 0)) {
			if (countItem(npc, 1) == 0)
				countContents(npc);
		}
	}
}

void ItemCounter::countDetached()
{
	objref cur = DetachedItems;
	while (cur.valid()) {
		if (countItem(cur, 0) != 0)
			break;
		if (!IS_NPC(ITEM(cur.off))) {
			countContents(cur);
		}
		cur = cur.next();
	}
}

/* Flags any NPC holding an item of type 0 in one of its 12 slots. */
void CheckEquippedTypes(int8_t *bad)
{
	int16_t j;
	int16_t unusedCount;
	objref held;
	objref npc;
	int16_t i;

	for (i = 0; i < NPC_COUNT; i++) {
		GetNpcIbo((objref *)&npc, i);
		if (npc.valid()) {
			unusedCount = 0;
			for (j = 0; j < 12; j++) {
				held = objref(GetItemInSlot(npc, j));
				if (held.valid() && ITEM(held.off)->typeFrame == 0)
					*bad = 1;
			}
		}
	}
}

void ItemCounter::run()
{
	clear();
	countFreeList();
	countDetached();
	countNpcs();
	countChunkLists();
	CheckEquippedTypes(&bad);
}

uint8_t VerifyItemBuffer(void)
{
	ItemCheck check;
	ItemCounter counter;
	int8_t ok;
	ok = 1;
	check.map = (uint8_t *)AllocateFarHeap((int32_t)ITEM_RECORDS, 0);
	if (check.map != 0) {
		check.run();
		if (check.bad != 0)
			ok = 0;
		FreeFarHeap(check.map);
	}
	if (ok != 0) {
		counter.run();
		if (counter.bad != 0 || counter.count != ITEM_RECORDS - FIRST_RECORD)
			ok = 0;
	}
	return ok;
}
