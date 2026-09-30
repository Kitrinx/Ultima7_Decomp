/* Black Gate U7.EXE, overlay segment 239 (file offsets 0x06da80 to 0x06e66c, 3052 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "lowlevel.h"
#include "dosio.h"
#include "item.h"
#include "coord.h"
#include "u7npc.h"
#include "cast.h"
#include "oops.h"
#include "type.h"
#include "npcref.h"
#include "equip.h"
#include "voonpc.h"

/* Saved records omit the live next link. */
#define ITEM_DATA(off) ItemAt((off) + sizeof(int16_t))
#define CONTENTS(off) ((Contents *)ItemAt(off))
#define ITEM_TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[ITEM_TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define HAS_CONTENTS(rec) ((int8_t)(CLASS_FLAGS(rec) & CLASS_CONTENTS))
#define IS_NPC(rec) ((int8_t)((CLASS_FLAGS(rec) & CLASS_NPC) != 0))
#define FRAME(rec) (((rec)->typeFrame & 0x7c00) >> 10)

#define MAX_STACK 100

struct Contents {
	int16_t first;
};

extern int16_t NpcItemRefs[];

inline uint8_t IsInContainer(objref *r)
{
	ItemInfo info = GetItemZAndStuff(r);

	return info.kind() >= LOCATION_CONTAINED;
}

char *MlmFileName = "mlm.txt";

/* Moves all of source's contents into dest when dest holds nothing; an NPC source loses its
 * equipment slots too. */
int8_t MoveContents(objref *source, objref *dest)
{
	int16_t from, to;
	objref current;
	int16_t npc, i;

	if (HAS_CONTENTS(ITEM(source->off)) && HAS_CONTENTS(ITEM(dest->off))) {
		from = ITEM(source->off)->data.extra;
		to = ITEM(dest->off)->data.extra;
		if (IS_NPC(ITEM(source->off))) {
			objref actor;
			actor = objref(source->off);
			npc = Item_getNpcNumber(&actor);
			if (npc != -1) {
				for (i = 0; i < 12; i++) {
					PokeWord(EquipList + npc * 24 + i * 2, 0);
				}
			}
			Item_clearWeaponReady(*source);
		}
		if (CONTENTS(to)->first == 0) {
			current.off = CONTENTS(from)->first;
			CONTENTS(to)->first = current.off;
			CONTENTS(from)->first = 0;
			while (current.valid()) {
				Item_setContainer(&current, dest->off);
				Item_setLocationKind(&current, LOCATION_CONTAINED);
				current = current.next();
			}
			return 1;
		}
	}
	return 0;
}

uint8_t CloneItem(objref *source)
{
	objref copy;
	uint16_t saved;
	int16_t npc;

	if (IS_NPC(ITEM(source->off))) {
		CreateNpc(&copy, TypeFrame(ITEM(source->off)->typeFrame), 0, 0, 0);
		npc = Item_getNpcNumber(&copy);
		saved = GetCachedNpcBuffer(&NpcPool, npc)->nextFree;
		MoveFarMemory(GetCachedNpcBuffer(&NpcPool, npc),
			GetCachedNpcBuffer(&NpcPool, NPCRef(*source).number()), sizeof(struct NpcBuffer));
		GetCachedNpcBuffer(&NpcPool, npc)->nextFree = saved;
		NpcItemRefs[npc] = copy.off;
		if (!Item_detach(&copy)) {
			copy.off = 0;
		}
	} else {
		CreateItem(&copy, TypeFrame(ITEM(source->off)->typeFrame));
	}
	if (copy.valid()) {
		SetItemZAndStuff(&copy, GetItemZAndStuff(source));
		Item_setHitPoints(&copy, Item_getHitPoints(source));
		Item_storeQuantity(&copy, (uint8_t)Item_getQuantity(source));
		Item_setQuality(&copy, (uint8_t)Item_getQuality(source));
		Item_setQualityFlags(&copy, Item_getQualityFlags(source));
		Item_setQualityFlags(&copy, Item_getQualityFlags(&copy) & ~0x20);
		if (Item_isOkayToTake(source)) {
			Item_setOkayToTake(&copy);
		}
		return 1;
	}
	return 0;
}

/* Whether second can join first's stack: the same quantity type and frame (unless the frame shows
 * the quantity), and at most MAX_STACK together. */
uint8_t CanStackWith(objref *first, objref second)
{
	int16_t type = ITEM_TYPE(ITEM(first->off));
	int16_t quantity;

	if (gItemTypeInfo[type].typeClass == TYPE_CLASS_QUANTITY && ITEM_TYPE(ITEM(second.off)) == (uint16_t)type &&
		(HasQuantityFrames(type) || FRAME(ITEM(first->off)) == FRAME(ITEM(second.off)))) {
		quantity = (uint8_t)Item_getQuantity(first) + (uint8_t)Item_getQuantity(&second);
		if (quantity <= MAX_STACK) {
			return 1;
		}
	}
	return 0;
}

uint8_t PlaceDraggedInContainer(objref *object, objref container)
{
	object->off = DetachedItems.off;
	if (object->valid() && container.valid()) {
		DetachedItems.off = ITEM(object->off)->next;
		objref last = GetContainedItem(&container);
		if (!last.valid()) {
			Item_linkIntoContainer(object, container);
		} else {
			Item_setLocationKind(object, LOCATION_CONTAINED);
			Item_setContainer(object, container);
			while (last.next().valid()) {
				last = last.next();
			}
			ITEM(last.off)->next = object->off;
			ITEM(object->off)->next = 0;
		}
		return 1;
	}
	return 0;
}

/* A length byte, then the item's records less the first one's next link: 6, 12 or 18 bytes. */
void WriteItemRecord(objref *object, int16_t file)
{
	uint8_t length;
	int16_t extra;

	switch (CLASS_FLAGS(ITEM(object->off)) & CLASS_RECORDS) {
	case 0:
		length = 6;
		DosWrite(file, -INT32_C(1), INT32_C(1), &length);
		DosWrite(file, -INT32_C(1), INT32_C(6), ITEM_DATA(object->off));
		break;
	case 1:
	case 2:
		length = 12;
		DosWrite(file, -INT32_C(1), INT32_C(1), &length);
		DosWrite(file, -INT32_C(1), INT32_C(4), ITEM_DATA(object->off));
		extra = ITEM(object->off)->data.extra;
		DosWrite(file, -INT32_C(1), INT32_C(8), ITEM(extra));
		break;
	case 3:
		length = 18;
		DosWrite(file, -INT32_C(1), INT32_C(1), &length);
		DosWrite(file, -INT32_C(1), INT32_C(4), ITEM_DATA(object->off));
		extra = ITEM(object->off)->data.extra;
		DosWrite(file, -INT32_C(1), INT32_C(6), ITEM(extra));
		extra = ITEM(extra)->data.extra;
		DosWrite(file, -INT32_C(1), INT32_C(8), ITEM(extra));
		break;
	}
}

/* Writes the list from object on, contents depth first, NPCs and type class 0 skipped. A 1 closes a
 * container's contents and a 0 ends the list. */
void WriteItemTree(objref *object, int16_t file)
{
	uint8_t marker, contained;
	objref child, parent, root;

	if (IsInContainer(object)) {
		root = Item_getContainer(object);
	} else {
		root.off = 0;
	}
	while (object->valid()) {
		if (IS_NPC(ITEM(object->off)) || (int8_t)(TYPE_CLASS(ITEM(object->off)) == TYPE_CLASS_NONE)) {
			object->off = ITEM(object->off)->next;
		} else {
			WriteItemRecord(object, file);
			child = GetContainedItem(object);
			if (child.valid()) {
				*object = child;
			} else {
				contained = IsInContainer(object);
				parent = Item_getContainer(object);
				for (object->off = ITEM(object->off)->next;
					!object->valid();
					object->off = ITEM(object->off)->next) {
					if (contained && parent != root) {
						marker = 1;
						DosWrite(file, -INT32_C(1), INT32_C(1), &marker);
						*object = parent;
						contained = IsInContainer(object);
						parent = Item_getContainer(object);
					} else {
						break;
					}
				}
			}
		}
	}
	marker = 0;
	DosWrite(file, -INT32_C(1), INT32_C(1), &marker);
}

void ReadItemRecord(objref *object, int16_t file, int16_t length, objref link, int16_t mode)
{
	int16_t extra;
	uint8_t data[18];

	DosRead(file, -INT32_C(1), length, data);
	AllocateItemRecords(object, length / 6U);
	if (!object->valid()) {
		return;
	}
	if (length == 6) {
		MoveFarMemory(ITEM_DATA(object->off), data, 6);
		if ((uint16_t)GetItemRecordCount(CLASS_FLAGS(ITEM(object->off))) != 1) {
			ReportInvalidSaveGame();
		}
	} else if (length == 12) {
		MoveFarMemory(ITEM_DATA(object->off), data, 4);
		MoveFarMemory(ITEM(ITEM(object->off)->data.extra), data + 4, 8);
		if ((uint16_t)GetItemRecordCount(CLASS_FLAGS(ITEM(object->off))) != 2) {
			ReportInvalidSaveGame();
		}
	} else {
		MoveFarMemory(ITEM_DATA(object->off), data, 4);
		extra = ITEM(object->off)->data.extra;
		MoveFarMemory(ITEM(extra), data + 4, 6);
		MoveFarMemory(ITEM(ITEM(extra)->data.extra), data + 10, 8);
		if ((uint16_t)GetItemRecordCount(CLASS_FLAGS(ITEM(object->off))) != 3) {
			ReportInvalidSaveGame();
		}
	}
	ITEM(object->off)->next = 0;
	ITEM(link.off)->next = object->off;
	if (IsOnMap(object)) {
		Item_setLocationKind(object, mode);
	}
}

/* Reads what WriteItemTree wrote: into object's contents when mode is LOCATION_CONTAINED, else after
 * object, placed in region slot mode. */
void ReadItemTree(objref *object, int16_t file, int16_t mode)
{
	objref link, parent, root;
	uint8_t length;

	if (mode == LOCATION_CONTAINED) {
		parent = *object;
		root = parent;
		link.off = Item_getExtraRecord(object);
	} else {
		link = *object;
		parent.off = 0;
		root.off = 0;
	}
	for (;;) {
		if (DosRead(file, -INT32_C(1), INT32_C(1), &length) != INT32_C(1)) {
			ReportInvalidSaveGame();
		}
		if (length == 0 || length == 1) {
			if (parent == root) {
				return;
			}
			ITEM(link.off)->next = 0;
			link = parent;
			parent = Item_getContainer(&parent);
		} else if ((int16_t)length == 6 || (int16_t)length == 12 || (int16_t)length == 18) {
			ReadItemRecord(object, file, length, link, mode);
			link = *object;
			if (parent.valid()) {
				Item_setContainer(object, parent);
			}
			if (GetContainedItem(object).valid()) {
				parent = *object;
				link.off = Item_getExtraRecord(object);
			}
		} else {
			ReportInvalidSaveGame();
			return;
		}
	}
}

void RestoreAvatarMana()
{
	GetNpcBufferForIbo(&AvatarRef)->mana = GetNpcBufferForIbo(&AvatarRef)->magic;
}

extern "C" void ResetItemovr1Globals(void)
{
	MlmFileName = "mlm.txt";
}
