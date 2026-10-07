/* Serpent Isle SI.EXE, resident segment 59 (file offsets 0x026969 to 0x0292bd, 10580 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "item.h"
#include "npcref.h"
#include "itembuf.h"
#include "sortitem.h"
#include "combat.h"
#include "u7npc.h"
#include "collide.h"
#include "init.h"
#include "lowlevel.h"
#include "slime.h"
#include "voonpc.h"
#include "type.h"
#include "coord.h"
#include "maps.h"
#include "mapview.h"
#include "iteminfo.h"
#include "equip.h"

ItemRecord *ItemBuffer = 0;
uint8_t *ItemBufferBase = 0;
int16_t ItemFreeList = 0;
objref DetachedItems;
int16_t ItemFreeCount = 0;
int16_t OffMapItemLink;
int16_t ChunkItemLists[4][16][16];

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define RECORD_KIND(rec) (*(uint8_t *)&ItemTypeClassFlags[TYPE_CLASS(rec)] & CLASS_RECORDS)
#define TYPE_FLAGS(ref) CLASS_FLAGS(ITEM((ref)->off))
#define IS_VALID(ref) ((int8_t)((ref) != 0))
#define IS_CLASS(rec, n) ((uint8_t)(TYPE_CLASS(rec) == (n)))
#define HAS_TYPE_CLASS(rec) ((uint8_t)!IS_CLASS(rec, TYPE_CLASS_NONE))
#define IS_NPC(rec) ((uint8_t)((CLASS_FLAGS(rec) & CLASS_NPC) != 0))

inline uint8_t GetLocationKind(ItemInfo &info) { return (int16_t)info.flags & 7; }
inline int8_t IsContained(ItemInfo &info) { return GetLocationKind(info) >= LOCATION_CONTAINED; }
inline int8_t IsOnMap(ItemInfo &info) { return GetLocationKind(info) <= 3; }
inline uint8_t IsEquipped(ItemInfo &info) { return info.kind() == LOCATION_EQUIPPED; }
inline uint8_t GetLocationKind(ItemInfo &&info) { return GetLocationKind(info); }
inline int8_t IsContained(ItemInfo &&info) { return IsContained(info); }
inline int8_t IsOnMap(ItemInfo &&info) { return IsOnMap(info); }
inline uint8_t IsEquipped(ItemInfo &&info) { return IsEquipped(info); }

inline uint8_t HasStrangeMovement(objref *ref)
{
	return gItemTypeInfo[ITEM(ref->off)->typeFrame & 0x3ff].strangeMovement;
}

inline uint8_t MoveItem(objref *ref, CellCoord x, CellCoord y)
{
	return Item_move(ref, x, y);
}

inline uint8_t MoveItem(objref *ref, Loc x, Loc y, int16_t z)
{
	return Item_move(ref, x, y, z);
}

inline uint8_t MoveDirect(objref *ref, Loc x, Loc y, int16_t z)
{
	return Item_forceMove(ref, x, y, z);
}

inline void MoveDirect(objref *ref, uint8_t dir, int16_t dz)
{
	Item_forceMove(ref, dir, dz);
}

inline uint8_t DirectionTo(objref &item, Coord x, Coord y, uint8_t cardinal)
{
	return Item_getDirToCoords(&item, x, y, cardinal);
}

inline uint8_t DirectionTo(objref &item, objref other, uint8_t cardinal)
{
	return Item_getDirToItem(&item, other, cardinal);
}

void Item_setFrame(objref *ref, int16_t frame)
{
	uint16_t &typeFrame = ITEM(ref->off)->typeFrame;

	typeFrame = (typeFrame & 0x83ff) | ((frame << 10) & 0x7c00);
}

Coord Item_getX(objref &ref)
{
	Coord result;
	ItemInfo info = GetItemZAndStuff(&ref);

	if (IsContained(info)) {
		return Item_getX(Item_getContainer(&ref));
	} else {
		result.value = ITEM(ref.off)->position.world.x + RegionX[Item_getRegion(&ref)].value;
		return result;
	}
}

Coord Item_getX(objref &&ref) { return Item_getX(ref); }

Coord Item_getY(objref &ref)
{
	Coord result;
	ItemInfo info = GetItemZAndStuff(&ref);

	if (IsContained(info)) {
		return Item_getY(Item_getContainer(&ref));
	} else {
		result.value = ITEM(ref.off)->position.world.y + RegionY[Item_getRegion(&ref)].value;
		return result;
	}
}

Coord Item_getY(objref &&ref) { return Item_getY(ref); }

void Item_getXAndY(objref &ref, int16_t *x, int16_t *y)
{
	ItemInfo info = GetItemZAndStuff(&ref);

	if (IsContained(info)) {
		Item_getXAndY(Item_getContainer(&ref), x, y);
	} else {
		ItemRecord *record = ITEM(ref.off);
		uint8_t region = Item_getRegion(&ref);
		*x = record->position.world.x + RegionX[region].value;
		*y = record->position.world.y + RegionY[region].value;
	}
}

ItemInfo GetItemZAndStuff(objref *ref)
{
	uint8_t records;
	int16_t extra;

	records = (int8_t)CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS;
	if (records == 0) {
		return ITEM(ref->off)->data.direct.z;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		return EXTRA(extra)->z;
	} else {
		return 0;
	}
}

void SetItemZAndStuff(objref *ref, ItemInfo &info)
{
	uint8_t records;
	int16_t extra;

	records = (int8_t)CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS;
	if (records == 0) {
		ITEM(ref->off)->data.direct.z = info.flags;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z = info.flags;
	}
}

void Item_setTemporary(objref *ref)
{
	OrItemZAndStuff(ref, 8);
}

void Item_clearTemporary(objref *ref)
{
	MaskItemZAndStuff(ref, 0xf7);
}

int16_t Item_hasHitPoints(objref *ref)
{
	return CLASS_FLAGS(ITEM(ref->off)) & CLASS_HIT_POINTS;
}

uint8_t Item_hasQuantity(objref *ref)
{
	return (CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUANTITY) &&
		!((uint8_t)((CLASS_FLAGS(ITEM(ref->off)) & CLASS_NPC) != 0));
}

uint8_t Item_hasQuality(objref *ref)
{
	return CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY;
}

uint8_t Item_isOkayToTake(objref *ref)
{
	int16_t extra;
	int16_t flag;

	if ((uint8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		return Item_getQualityFlags(ref) & QUALITY_OKAY_TO_TAKE;
	}
	if ((uint8_t)Item_hasQuantity(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			flag = EXTRA(extra)->flags & 0x80;
		} else {
			flag = ITEM(ref->off)->data.direct.quality & 0x80;
		}
		return flag != 0;
	}
	return 1;
}

void Item_setOkayToTake(objref *ref)
{
	int16_t extra;

	if ((uint8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_OKAY_TO_TAKE);
	} else if ((uint8_t)Item_hasQuantity(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags |= 0x80;
		} else {
			ITEM(ref->off)->data.direct.quality |= 0x80;
		}
	}
}

void Item_clearOkayToTake(objref *ref)
{
	int16_t extra;

	if ((uint8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		Item_setQualityFlags(ref, Item_getQualityFlags(ref) & ~QUALITY_OKAY_TO_TAKE);
	} else if ((uint8_t)Item_hasQuantity(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags &= ~0x80;
		} else {
			ITEM(ref->off)->data.direct.quality &= ~0x80;
		}
	}
}

void Item_setWeaponReady(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_WEAPON_READY);
}

void Item_clearWeaponReady(objref &ref)
{
	Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) & ~QUALITY_WEAPON_READY);
}

void Item_setCarriesLight(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_CARRIES_LIGHT);
}

void Item_clearCarriesLight(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) & 0x7f);
}

void Item_setLocationKind(objref *ref, uint8_t value)
{
	uint8_t records;
	int16_t extra;

	records = RECORD_KIND(ITEM(ref->off));
	value &= 7;
	if (records == 0) {
		ITEM(ref->off)->data.direct.z &= ~7;
		ITEM(ref->off)->data.direct.z |= value;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z &= ~7;
		EXTRA(extra)->z |= value;
	}
}

void MaskItemZAndStuff(objref *ref, uint8_t value)
{
	uint8_t records;
	int16_t extra;

	records = RECORD_KIND(ITEM(ref->off));
	if (records == 0) {
		ITEM(ref->off)->data.direct.z &= value;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z &= value;
	}
}

void OrItemZAndStuff(objref *ref, uint8_t value)
{
	uint8_t records;
	int16_t extra;

	records = RECORD_KIND(ITEM(ref->off));
	if (records == 0) {
		ITEM(ref->off)->data.direct.z |= value;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z |= value;
	}
}

void Item_markEquipped(objref *ref)
{
	ItemInfo info = GetItemZAndStuff(ref);

	if (IsContained(info)) {
		Item_setLocationKind(ref, LOCATION_EQUIPPED);
	}
}

void Item_unequip(objref *ref)
{
	ItemInfo info = GetItemZAndStuff(ref);
	int16_t container;
	uint8_t slot;
	int16_t npc;

	if ((int8_t)((uint8_t)((uint16_t)info.flags & 7) == LOCATION_EQUIPPED)) {
		Item_setLocationKind(ref, LOCATION_CONTAINED);
		container = Item_getContainer(ref);
		npc = Item_getNpcNumber(&NPCRef(container));
		if (npc != -1) {
			for (slot = 0; slot < 18; slot++) {
				if (PeekWord(EquipList + npc * 36 + slot * 2) == ref->off) {
					PokeWord(EquipList + npc * 36 + slot * 2, 0);
					if (slot == 1) {
						Item_clearWeaponReady(Item_getContainer(ref));
					}
				}
			}
		}
	}
}

int8_t Item_getHitPoints(objref *ref)
{
	int16_t extra;

	if ((uint8_t)Item_hasHitPoints(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->hitPoints;
		} else {
			return ITEM(ref->off)->data.direct.quality;
		}
	} else {
		return 0;
	}
}

void Item_setHitPoints(objref *ref, uint8_t value)
{
	int16_t extra;

	if ((uint8_t)Item_hasHitPoints(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->hitPoints = value;
		} else {
			ITEM(ref->off)->data.direct.quality = value;
		}
	}
}

uint8_t Item_getQualityFlags(objref *ref)
{
	int16_t extra;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->qualityFlags;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 0;
}

void Item_setQualityFlags(objref *ref, uint8_t value)
{
	int16_t extra;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->qualityFlags = value;
		} else {
			ITEM(ref->off)->data.direct.quality = value;
		}
	}
}

uint8_t Item_getQuantity(objref *ref)
{
	int16_t extra;

	if ((int8_t)Item_hasQuantity(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->flags & 0x7f;
		} else {
			return ITEM(ref->off)->data.direct.quality & 0x7f;
		}
	}
	return 1;
}

void Item_storeQuantity(objref *ref, uint8_t quantity)
{
	int16_t extra;

	if ((int8_t)Item_hasQuantity(ref)) {
		quantity &= 0x7f;
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags = (EXTRA(extra)->flags & 0x80) | quantity;
		} else {
			ITEM(ref->off)->data.direct.quality = (ITEM(ref->off)->data.direct.quality & 0x80) | quantity;
		}
	}
}

uint8_t Item_getQuality(objref *ref)
{
	int16_t extra;

	if ((int8_t)Item_hasQuality(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->quality;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 0;
}

uint8_t Item_getCharges(objref *ref)
{
	if ((int8_t)Item_hasQuality(ref)) {
		return Item_getQuality(ref);
	} else {
		return 1;
	}
}

void Item_setQuality(objref *ref, int8_t quality)
{
	int16_t extra;

	if ((int8_t)Item_hasQuality(ref)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->quality = quality;
		} else {
			ITEM(ref->off)->data.direct.quality = quality;
		}
	}
}

uint8_t Item_getRegion(objref *ref)
{
	int16_t kind;
	int16_t extra;

	kind = GetLocationKind(GetItemZAndStuff(ref));
	if (kind <= 3)
		return LoadedRegions[kind];
	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_REGION)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->region;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 255;
}

void Item_setRegion(objref *ref, uint8_t region)
{
	int16_t extra;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_REGION)) {
		if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->region = region;
		} else {
			ITEM(ref->off)->data.direct.quality = region;
		}
	}
}

int16_t GetItemRecordCount(int16_t classFlags)
{
	int16_t count = classFlags & CLASS_RECORDS;

	if (!count)
		count++;
	return count;
}

void AllocateItemRecords(objref *ref, int16_t count)
{
	objref record;
	int16_t i;

	ref->off = 0;
	if (ItemFreeCount >= count) {
		ItemFreeCount -= count;
		ref->off = ItemFreeList;
		ItemFreeList = ITEM(ItemFreeList)->next;
		record.off = ref->off;
		for (i = 1; i < count; i++) {
			ITEM(record.off)->data.extra = ItemFreeList;
			record.off = ItemFreeList;
			ItemFreeList = ITEM(ItemFreeList)->next;
		}
	}
}

int16_t FindRegionSlot(Loc x, Loc y)
{
	uint8_t region;
	int16_t i;

	region = GetRegionAt(x, y, CurrentMap);
	for (i = 0; i < 4; i++)
		if (LoadedRegions[i] == region)
			return i;
	return 255;
}

inline int16_t GetContentsOf(objref *container)
{
	objref contents = GetContainedItem(container);

	return contents.off;
}

void Item_linkIntoContainer(objref *ref, objref container)
{
	Item_setLocationKind(ref, LOCATION_CONTAINED);
	ITEM(ref->off)->next = GetContentsOf(&container);
	Item_setContents(&container, *ref);
	Item_setContainer(ref, container);
}

uint8_t CreateItem(objref *ref, TypeFrame type, CellCoord x, CellCoord y, int16_t z)
{
	if (CreateItem(ref, type)) {
		if (!PlaceItem(ref, x, y, z)) {
			Item_setLocationKind(ref, LOCATION_CONTAINED);
			ZapDetachedItem(ref);
		} else {
			return 1;
		}
	}
	return 0;
}

uint8_t CreateItemInContainer(objref *ref, TypeFrame type, objref container)
{
	if (container.valid() && (int8_t)(CLASS_FLAGS(ITEM(container.off)) & CLASS_CONTENTS)) {
		if (CreateItem(ref, type)) {
			return PlaceItemInContainer(ref, container);
		}
	}
	return 0;
}

int16_t * Item_findLink(objref *ref)
{
	int16_t *link = 0;
	uint8_t kind;
	int16_t x, y;

	if (ref->valid()) {
		kind = GetLocationKind(GetItemZAndStuff(ref));
		if (kind <= 3) {
			x = ITEM(ref->off)->position.world.x >> 4;
			y = ITEM(ref->off)->position.world.y >> 4;
			link = &ChunkItemLists[kind][y][x];
		} else if (kind >= LOCATION_CONTAINED) {
			link = &EXTRA(ITEM(Item_getContainer(ref).off)->data.extra)->contents;
		} else if (kind == LOCATION_OFF_MAP) {
			link = &OffMapItemLink;
			OffMapItemLink = ref->off;
		}
		if (link && *link != ref->off) {
			while (*link && *link != ref->off)
				link = &ITEM(*link)->next;
			if (!*link)
				link = 0;
		}
	}
	return link;
}

int8_t Item_spillContents(objref *ref)
{
	int8_t contained;
	objref current, container;
	Coord x, y;
	int16_t z;

	if (IsContained(GetItemZAndStuff(ref))) {
		contained = 1;
		container = Item_getContainer(ref);
	} else {
		contained = 0;
		x = Item_getX(*ref);
		y = Item_getY(*ref);
		z = GetItemZAndStuff(ref).z();
	}
	while ((current = GetContainedItem(ref)).valid()) {
		if (contained) {
			Item_moveIntoContainer(&current, container);
		} else {
			Item_move(&current, x, y, z);
		}
	}
	return 0;
}

int8_t Item_deleteContents(objref *ref)
{
	objref current;

	while ((current = GetContainedItem(ref)).valid())
		Item_delete(&current);
	return 0;
}

objref Item_getContainer(objref *ref)
{
	objref parent = 0;

	if (IsContained(GetItemZAndStuff(ref)))
		parent.off = ITEM(ref->off)->position.parent;
	return parent;
}

void Item_setContainer(objref *ref, objref parent)
{
	if (IsContained(GetItemZAndStuff(ref)))
		ITEM(ref->off)->position.parent = parent.off;
}

objref GetContainedItem(objref *ref)
{
	objref contents = 0;
	int16_t extra;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS)) {
		extra = ITEM(ref->off)->data.extra;
		contents.off = EXTRA(extra)->contents;
	}
	return contents;
}

int16_t Item_getExtraRecord(objref *ref)
{
	int16_t extra = 0;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS))
		extra = ITEM(ref->off)->data.extra;
	return extra;
}

void Item_setContents(objref *ref, objref contents)
{
	int16_t extra;

	if ((int8_t)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS)) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->contents = contents.off;
	}
}

uint8_t Item_isWithin(objref *ref, objref container)
{
	objref saved;
	uint8_t found = 0;

	saved = *ref;
	while (IsContained(GetItemZAndStuff(ref))) {
		objref parent = Item_getContainer(ref);
		*ref = parent;
		if (ref->off == container.off)
			found = 1;
	}
	*ref = saved;
	return found;
}

void FreeItemRecord(objref *ref)
{
	int16_t *record = (int16_t *)ITEM(ref->off);

	*record = ItemFreeList;
	*++record = 0;
	*++record = 0;
	*++record = 0;
	ItemFreeList = ref->off;
	ItemFreeCount++;
}

int8_t Item_delete(objref *ref)
{
	if (GetLocationKind(GetItemZAndStuff(ref)) == LOCATION_DELETED)
		return 0;
	if (ITEM(ref->off)->typeFrame == 0)
		return 1;
	if (ref->off == DetachedItems.off)
		return ZapDetachedItem(ref);
	Item_spillContents(ref);
	if (Item_detach(ref))
		return ZapDetachedItem(ref);
	return 0;
}

void Item_setZ(objref *ref, int16_t z)
{
	int16_t x, y;

	if (GetItemZAndStuff(ref).z() != z) {
		if (IsOnMap(GetItemZAndStuff(ref))) {
			x = Item_getX(*ref).value;
			y = Item_getY(*ref).value;
			Item_detach(ref);
			MaskItemZAndStuff(ref, 0x0f);
			OrItemZAndStuff(ref, (uint8_t)z << 4);
			PlaceItem(ref, x, y);
		}
	}
}

uint8_t Item_move(objref *ref, CellCoord x, CellCoord y)
{
	if (!ref->valid())
		return 0;
	if (HasStrangeMovement(ref)) {
		if (!OnStrangeMoverStep(*ref, x, y, GetItemZAndStuff(ref).z()))
			return 0;
	}
	if (FindRegionSlot(x, y) == 255 && !ref->isNpc())
		return 0;
	if (Item_detach(ref)) {
		if (!PlaceItem(ref, x, y)) {
			if (!PlaceItem(ref, Item_getX(*ref), Item_getY(*ref))) {
				Item_setLocationKind(ref, LOCATION_CONTAINED);
				ZapDetachedItem(ref);
				return 0;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

uint8_t Item_move(objref *ref, Loc x, Loc y, int16_t z)
{
	if (!ref->valid())
		return 0;
	if (HasStrangeMovement(ref)) {
		if (!OnStrangeMoverStep(*ref, x, y, z))
			return 0;
	}
	if (z < 0 || z > 15)
		return 0;
	if (FindRegionSlot(x, y) == 255 && !ref->isNpc())
		return 0;
	if (Item_detach(ref)) {
		if (!PlaceItem(ref, x, y, z)) {
			if (!PlaceItem(ref, Item_getX(*ref), Item_getY(*ref))) {
				Item_setLocationKind(ref, LOCATION_CONTAINED);
				ZapDetachedItem(ref);
				return 0;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

uint8_t Item_forceMove(objref *ref, Loc x, Loc y, int16_t z)
{
	if (!ref->valid())
		return 0;
	if (z < 0 || z > 15)
		return 0;
	if (Item_detach(ref)) {
		if (!PlaceItemDirect(ref, x, y, z)) {
			if (!PlaceItem(ref, Item_getX(*ref), Item_getY(*ref))) {
				Item_setLocationKind(ref, LOCATION_CONTAINED);
				ZapDetachedItem(ref);
				return 0;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

void Item_move(objref *ref, uint8_t dir)
{
	MoveItem(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir]);
}

void Item_move(objref *ref, uint8_t dir, int16_t dz)
{
	MoveItem(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir],
		GetItemZAndStuff(ref).z() + dz);
}

void Item_forceMove(objref *ref, uint8_t dir, int16_t dz)
{
	MoveDirect(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir],
		GetItemZAndStuff(ref).z() + dz);
}

uint8_t Item_moveIntoContainer(objref *ref, objref container)
{
	if (Item_detach(ref)) {
		if (!PlaceItemInContainer(ref, container)) {
			if (!PlaceItem(ref, Item_getX(*ref), Item_getY(*ref))) {
				Item_setLocationKind(ref, LOCATION_CONTAINED);
				ZapDetachedItem(ref);
				return 0;
			}
			return 0;
		}
		return 1;
	}
	return 0;
}

uint8_t Item_detach(objref *ref)
{
	int16_t *link;
	uint8_t notify;
	int16_t kind;

	notify = HasStrangeMovement(ref) && !Item_getContainer(ref).valid();
	link = Item_findLink(ref);
	if (link) {
		kind = GetItemZAndStuff(ref).kind();
		if (kind <= 3) {
			RemoveTypeFromCollision(*ref);
		} else if (IsEquipped(GetItemZAndStuff(ref))) {
			Item_unequip(ref);
		}
		*link = ITEM(ref->off)->next;
		ITEM(ref->off)->next = DetachedItems.off;
		DetachedItems.off = ref->off;
		if ((uint8_t)(TYPE_FLAGS(ref) & CLASS_REGION) && kind <= 3)
			Item_setRegion(ref, LoadedRegions[kind]);
		if (notify) {
			OnStrangeMoverRemoved(*ref);
		}
		return 1;
	}
	return 0;
}

uint8_t CreateItem(objref *ref, TypeFrame frame)
{
	objref empty;
	int16_t flags, count;

	flags = ItemTypeClassFlags[gItemTypeInfo[frame.bits & 0x3ff].typeClass];
	count = GetItemRecordCount(flags);
	AllocateItemRecords(ref, count);
	if (ref->valid()) {
		ITEM(ref->off)->setTypeFrame(frame);
		Item_clearTemporary(ref);
		Item_setQualityFlags(ref, 0);
		if ((uint8_t)(TYPE_FLAGS(ref) & CLASS_CONTENTS)) {
			empty.off = 0;
			Item_setContents(ref, empty);
		}
		ITEM(ref->off)->next = DetachedItems.off;
		DetachedItems.off = ref->off;
		return 1;
	}
	return 0;
}

uint8_t PlaceItemDirect(objref *ref, Loc x, Loc y, int16_t z)
{
	int16_t *head;
	int16_t chunkX, chunkY, regionIndex;

	ref->off = DetachedItems.off;
	if (ref->valid()) {
		MaskItemZAndStuff(ref, 15);
		OrItemZAndStuff(ref, (uint8_t)z << 4);
		regionIndex = FindRegionSlot(x, y);
		if (regionIndex != 255) {
			chunkX = (x.value & 0xff) >> 4;
			chunkY = (y.value & 0xff) >> 4;
			head = &ChunkItemLists[regionIndex][chunkY][chunkX];
			DetachedItems.off = ITEM(ref->off)->next;
			ITEM(ref->off)->position.world.x = x;
			ITEM(ref->off)->position.world.y = y;
			Item_setLocationKind(ref, regionIndex);
			ITEM(ref->off)->next = *head;
			*head = ref->off;
			if ((uint8_t)(TYPE_FLAGS(ref) & CLASS_REGION))
				Item_setRegion(ref, LoadedRegions[regionIndex]);
			return 1;
		} else if (ref->isNpc()) {
			DetachedItems.off = ITEM(ref->off)->next;
			ITEM(ref->off)->next = 0;
			Item_setLocationKind(ref, LOCATION_OFF_MAP);
			Item_setRegion(ref, GetRegionAt(x, y, CurrentMap));
			return 1;
		}
	}
	return 0;
}

uint8_t PlaceItem(objref *ref, Loc x, Loc y, int16_t z)
{
	ref->off = DetachedItems.off;
	if (ref->valid()) {
		MaskItemZAndStuff(ref, 15);
		OrItemZAndStuff(ref, (uint8_t)z << 4);
		return PlaceItem(ref, x, y);
	}
	return 0;
}

uint8_t PlaceItemInContainer(objref *ref, objref container)
{
	ref->off = DetachedItems.off;
	if (ref->valid()) {
		DetachedItems.off = ITEM(ref->off)->next;
		Item_linkIntoContainer(ref, container);
		return 1;
	}
	return 0;
}

uint8_t PlaceItemOffMap(objref *ref)
{
	ref->off = DetachedItems.off;
	if (ref->valid()) {
		DetachedItems.off = ITEM(ref->off)->next;
		ITEM(ref->off)->next = 0;
		Item_setLocationKind(ref, LOCATION_OFF_MAP);
		return 1;
	}
	return 0;
}

uint8_t ZapDetachedItem(objref *ref)
{
	objref npc;
	uint8_t records;
	uint8_t result = 0;
	ItemInfo info = GetItemZAndStuff(ref);

	if (GetLocationKind(info) == LOCATION_DELETED)
		return 0;
	if (ITEM(ref->off)->typeFrame == 0)
		return 1;
	ref->off = DetachedItems.off;
	Item_setLocationKind(ref, LOCATION_DELETED);
	if (ref->valid()) {
		if (HAS_TYPE_CLASS(ITEM(ref->off)) && !IS_CLASS(ITEM(ref->off), TYPE_CLASS_BUILDING))
			DetachItem(ref->off);
		DetachedItems.off = ITEM(ref->off)->next;
		if (IS_NPC(ITEM(ref->off))) {
			GetNpcIbo(&npc, Item_getNpcNumber(&NPCRef(ref->off)));
			if (npc == *ref) {
				if (IS_CLASS(ITEM(ref->off), TYPE_CLASS_MONSTER))
					Npc_freeMonsterNumber(&npc);
				else
					Npc_freeNumber(&npc);
				SaveNpcSlot(NpcSlotOf[Item_getNpcNumber(&npc)]);
			}
		}
		records = RECORD_KIND(ITEM(ref->off));
		if (records != 0) {
			objref extra = ITEM(ref->off)->data.extra;
			if (records == 3) {
				objref secondary = ITEM(extra.off)->data.extra;
				FreeItemRecord(&secondary);
			}
			FreeItemRecord(&extra);
		}
		FreeItemRecord(ref);
		ref->off = 0;
		result = 1;
	}
	return result;
}

int16_t GetItemBeingDragged(objref *ref)
{
	ref->off = DetachedItems.off;
	if (ref->off != 0) {
		return 1;
	}
	return 0;
}

uint8_t IsItemDetached(objref *ref)
{
	objref current = DetachedItems.off;

	while (IS_VALID(current.off)) {
		if (current == *ref)
			return 1;
		current = current.next();
	}
	return 0;
}

int16_t CountDetachedItems()
{
	int16_t count = 0;

	for (objref ref = DetachedItems.off; ref.valid(); ref = ref.next())
		count++;
	return count;
}

extern "C" int16_t Item_greatestDeltaToItem(objref &ref, objref other)
{
	Coord x1, y1, x2, y2;
	uint16_t z1, z2;

	Item_getXAndY(ref, &x1.value, &y1.value);
	z1 = Item_getZ(&ref);
	Item_getXAndY(other, &x2.value, &y2.value);
	z2 = Item_getZ(&other);
	if (x1 < x2)
		x2 = x2 - GetFootprintX(ITEM(other.off)->asTypeFrame());
	else if (x2 < x1)
		x1 = x1 - GetFootprintX(ITEM(ref.off)->asTypeFrame());
	if (y1 < y2)
		y2 = y2 - GetFootprintY(ITEM(other.off)->asTypeFrame());
	else if (y2 < y1)
		y1 = y1 - GetFootprintY(ITEM(ref.off)->asTypeFrame());
	if (z1 < z2) {
		if (z2 < z1 + gItemTypeInfo[TYPE(ITEM(ref.off))].height)
			z1 = z2;
		else
			z1 = z1 + gItemTypeInfo[TYPE(ITEM(other.off))].height;
	} else if (z2 < z1) {
		if (z1 < z2 + gItemTypeInfo[TYPE(ITEM(other.off))].height)
			z2 = z1;
		else
			z2 = z2 + gItemTypeInfo[TYPE(ITEM(ref.off))].height;
	}
	return GetDistance(x1, y1, z1, x2, y2, z2);
}

void Item_setInvisible(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_INVISIBLE);
	if ((int8_t)((CLASS_FLAGS(ITEM(ref->off)) & CLASS_NPC) != 0))
		CombatGroups.loseSightOf(ref->off);
}

int16_t Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, uint16_t targetZ)
{
	Coord x, y;

	Item_getXAndY(ref, &x.value, &y.value);
	uint16_t z = Item_getZ(&ref);

	if (targetX < x)
		x = x - GetFootprintX(ITEM(ref.off)->asTypeFrame()) + 1;
	if (targetY < y)
		y = y - GetFootprintY(ITEM(ref.off)->asTypeFrame()) + 1;
	if (targetZ < z)
		z = z - gItemTypeInfo[TYPE(ITEM(ref.off))].height + 1;
	return GetDistance(x, y, z, targetX, targetY, targetZ);
}

uint8_t Item_getDirToItem(objref *item, objref other, uint8_t cardinal)
{
	return DirectionTo(*item, Item_getX(other), Item_getY(other), cardinal);
}

uint8_t GetDiagonalStep(int16_t major, int16_t minor)
{
	if ((minor << 1) <= major) {
		return 0;
	} else {
		if ((major << 1) <= minor)
			return 2;
	}
	return 1;
}

uint8_t Item_getDirToCoords(objref *item, Coord x, Coord y, uint8_t cardinal)
{
	int16_t dx = x.value - Item_getX(*item).value;
	int16_t dy = y.value - Item_getY(*item).value;
	int16_t absX = dx < 0 ? -dx : dx;
	int16_t absY = dy < 0 ? -dy : dy;

	if (dx < 0) {
		if (dy < 0) {
			if (!cardinal)
				return ((int16_t)GetDiagonalStep(absX, absY) + 6) & 7;
			else if (absX > absY)
				return 6;
			else
				return 0;
		} else if (dy > 0) {
			if (!cardinal)
				return (int16_t)GetDiagonalStep(absY, absX) + 4;
			else if (absX > absY)
				return 6;
			else
				return 4;
		} else {
			return 6;
		}
	} else if (dx > 0) {
		if (dy < 0) {
			if (!cardinal)
				return (int16_t)GetDiagonalStep(absY, absX);
			else if (absX > absY)
				return 2;
			else
				return 0;
		} else if (dy > 0) {
			if (!cardinal)
				return (int16_t)GetDiagonalStep(absX, absY) + 2;
			else if (absX > absY)
				return 2;
			else
				return 4;
		} else {
			return 2;
		}
	} else if (dy < 0) {
		return 0;
	} else if (dy > 0) {
		return 4;
	} else {
		return 8;
	}
}

void CheckItemHandle(uint16_t handle)
{
	if ((handle & 7) != 0 || handle > ItemBufferBytes)
		FatalError("Item corrupt.\n");
}

void CheckItemLists(void)
{
	objref current;
	int16_t k, j, i;

	for (k = 0; k < 4; k++)
		for (j = 0; j < 16; j++)
			for (i = 0; i < 16; i++) {
				current = ChunkItemLists[k][j][i];
				while (IS_VALID(current.off)) {
					if ((current.off & 7) != 0 || (uint16_t)(current.off) > ItemBufferBytes)
						FatalError("Item list corrupt.\n");
					current = current.next();
				}
			}
}

void ReportBadFreeList(char *file, int16_t line)
{
	FatalError("Bad Free List @%s, %d", file, line);
}

extern "C" void ResetItemGlobals(void)
{
	ItemBuffer = 0;
	ItemBufferBase = 0;
	ItemFreeList = 0;
	DetachedItems = 0;
	ItemFreeCount = 0;
	OffMapItemLink = 0;
	memset(ChunkItemLists, 0, sizeof ChunkItemLists);
}
