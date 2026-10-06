/* Serpent Isle SI.EXE, resident segment 59 (file offsets 0x026969 to 0x0292bd, 10580 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 */

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

ItemRecord far *ItemBuffer = 0;
unsigned ItemBufferSegment = 0;
int ItemFreeList = 0;
objref DetachedItems;
int ItemFreeCount = 0;
int OffMapItemLink;
int ChunkItemLists[4][16][16];

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)
#define TYPE_CLASS(rec) (gItemTypeInfo[TYPE(rec)].typeClass)
#define CLASS_FLAGS(rec) (ItemTypeClassFlags[TYPE_CLASS(rec)])
#define RECORD_KIND(rec) (*(unsigned char *)&ItemTypeClassFlags[TYPE_CLASS(rec)] & CLASS_RECORDS)
#define TYPE_FLAGS(ref) CLASS_FLAGS(ITEM((ref)->off))
#define IS_VALID(ref) ((char)((ref) != 0))
#define IS_CLASS(rec, n) ((unsigned char)(TYPE_CLASS(rec) == (n)))
#define HAS_TYPE_CLASS(rec) ((unsigned char)!IS_CLASS(rec, TYPE_CLASS_NONE))
#define IS_NPC(rec) ((unsigned char)((CLASS_FLAGS(rec) & CLASS_NPC) != 0))

inline unsigned char GetLocationKind(ItemInfo far &info) { return (int)info.flags & 7; }
inline char IsContained(ItemInfo far &info) { return GetLocationKind(info) >= LOCATION_CONTAINED; }
inline char IsOnMap(ItemInfo far &info) { return GetLocationKind(info) <= 3; }
inline unsigned char IsEquipped(ItemInfo far &info) { return info.kind() == LOCATION_EQUIPPED; }

inline unsigned char HasStrangeMovement(objref *ref)
{
	return gItemTypeInfo[ITEM(ref->off)->typeFrame & 0x3ff].strangeMovement;
}

inline unsigned char MoveItem(objref *ref, CellCoord x, CellCoord y)
{
	return Item_move(ref, x, y);
}

inline unsigned char MoveItem(objref *ref, Loc x, Loc y, int z)
{
	return Item_move(ref, x, y, z);
}

inline unsigned char MoveDirect(objref *ref, Loc x, Loc y, int z)
{
	return Item_forceMove(ref, x, y, z);
}

inline void MoveDirect(objref *ref, unsigned char dir, int dz)
{
	Item_forceMove(ref, dir, dz);
}

inline unsigned char DirectionTo(objref &item, Coord x, Coord y, unsigned char cardinal)
{
	return Item_getDirToCoords(&item, x, y, cardinal);
}

inline unsigned char DirectionTo(objref &item, objref other, unsigned char cardinal)
{
	return Item_getDirToItem(&item, other, cardinal);
}

void far Item_setFrame(objref *ref, int frame)
{
	unsigned far &typeFrame = ITEM(ref->off)->typeFrame;

	typeFrame = (typeFrame & 0x83ff) | ((frame << 10) & 0x7c00);
}

Coord far Item_getX(objref &ref)
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

Coord far Item_getY(objref &ref)
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

void far Item_getXAndY(objref &ref, int *x, int *y)
{
	ItemInfo info = GetItemZAndStuff(&ref);

	if (IsContained(info)) {
		Item_getXAndY(Item_getContainer(&ref), x, y);
	} else {
		ItemRecord far *record = ITEM(ref.off);
		unsigned char region = Item_getRegion(&ref);
		*x = record->position.world.x + RegionX[region].value;
		*y = record->position.world.y + RegionY[region].value;
	}
}

ItemInfo far GetItemZAndStuff(objref *ref)
{
	unsigned char records;
	int extra;

	records = (char)CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS;
	if (records == 0) {
		return ITEM(ref->off)->data.direct.z;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		return EXTRA(extra)->z;
	} else {
		return 0;
	}
}

void far SetItemZAndStuff(objref *ref, ItemInfo far &info)
{
	unsigned char records;
	int extra;

	records = (char)CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS;
	if (records == 0) {
		ITEM(ref->off)->data.direct.z = info.flags;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z = info.flags;
	}
}

void far Item_setTemporary(objref *ref)
{
	OrItemZAndStuff(ref, 8);
}

void far Item_clearTemporary(objref *ref)
{
	MaskItemZAndStuff(ref, 0xf7);
}

int far Item_hasHitPoints(objref *ref)
{
	return CLASS_FLAGS(ITEM(ref->off)) & CLASS_HIT_POINTS;
}

unsigned char far Item_hasQuantity(objref *ref)
{
	return (CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUANTITY) &&
		!((unsigned char)((CLASS_FLAGS(ITEM(ref->off)) & CLASS_NPC) != 0));
}

unsigned char far Item_hasQuality(objref *ref)
{
	return CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY;
}

unsigned char far Item_isOkayToTake(objref *ref)
{
	int extra;
	int flag;

	if ((unsigned char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		return Item_getQualityFlags(ref) & QUALITY_OKAY_TO_TAKE;
	}
	if ((unsigned char)Item_hasQuantity(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			flag = EXTRA(extra)->flags & 0x80;
		} else {
			flag = ITEM(ref->off)->data.direct.quality & 0x80;
		}
		return flag != 0;
	}
	return 1;
}

void far Item_setOkayToTake(objref *ref)
{
	int extra;

	if ((unsigned char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_OKAY_TO_TAKE);
	} else if ((unsigned char)Item_hasQuantity(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags |= 0x80;
		} else {
			ITEM(ref->off)->data.direct.quality |= 0x80;
		}
	}
}

void far Item_clearOkayToTake(objref *ref)
{
	int extra;

	if ((unsigned char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		Item_setQualityFlags(ref, Item_getQualityFlags(ref) & ~QUALITY_OKAY_TO_TAKE);
	} else if ((unsigned char)Item_hasQuantity(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags &= ~0x80;
		} else {
			ITEM(ref->off)->data.direct.quality &= ~0x80;
		}
	}
}

void far Item_setWeaponReady(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_WEAPON_READY);
}

void far Item_clearWeaponReady(objref &ref)
{
	Item_setQualityFlags(&ref, Item_getQualityFlags(&ref) & ~QUALITY_WEAPON_READY);
}

void far Item_setCarriesLight(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_CARRIES_LIGHT);
}

void far Item_clearCarriesLight(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) & 0x7f);
}

void far Item_setLocationKind(objref *ref, unsigned char value)
{
	unsigned char records;
	int extra;

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

void far MaskItemZAndStuff(objref *ref, unsigned char value)
{
	unsigned char records;
	int extra;

	records = RECORD_KIND(ITEM(ref->off));
	if (records == 0) {
		ITEM(ref->off)->data.direct.z &= value;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z &= value;
	}
}

void far OrItemZAndStuff(objref *ref, unsigned char value)
{
	unsigned char records;
	int extra;

	records = RECORD_KIND(ITEM(ref->off));
	if (records == 0) {
		ITEM(ref->off)->data.direct.z |= value;
	} else if (records >= 2) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->z |= value;
	}
}

void far Item_markEquipped(objref *ref)
{
	ItemInfo info = GetItemZAndStuff(ref);

	if (IsContained(info)) {
		Item_setLocationKind(ref, LOCATION_EQUIPPED);
	}
}

void far Item_unequip(objref *ref)
{
	ItemInfo info = GetItemZAndStuff(ref);
	int container;
	unsigned char slot;
	int npc;

	if ((char)((unsigned char)((unsigned)info.flags & 7) == LOCATION_EQUIPPED)) {
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

char far Item_getHitPoints(objref *ref)
{
	int extra;

	if ((unsigned char)Item_hasHitPoints(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->hitPoints;
		} else {
			return ITEM(ref->off)->data.direct.quality;
		}
	} else {
		return 0;
	}
}

void far Item_setHitPoints(objref *ref, unsigned char value)
{
	int extra;

	if ((unsigned char)Item_hasHitPoints(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->hitPoints = value;
		} else {
			ITEM(ref->off)->data.direct.quality = value;
		}
	}
}

unsigned char far Item_getQualityFlags(objref *ref)
{
	int extra;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->qualityFlags;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 0;
}

void far Item_setQualityFlags(objref *ref, unsigned char value)
{
	int extra;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_QUALITY_FLAGS)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->qualityFlags = value;
		} else {
			ITEM(ref->off)->data.direct.quality = value;
		}
	}
}

unsigned char far Item_getQuantity(objref *ref)
{
	int extra;

	if ((char)Item_hasQuantity(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->flags & 0x7f;
		} else {
			return ITEM(ref->off)->data.direct.quality & 0x7f;
		}
	}
	return 1;
}

void far Item_storeQuantity(objref *ref, unsigned char quantity)
{
	int extra;

	if ((char)Item_hasQuantity(ref)) {
		quantity &= 0x7f;
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->flags = (EXTRA(extra)->flags & 0x80) | quantity;
		} else {
			ITEM(ref->off)->data.direct.quality = (ITEM(ref->off)->data.direct.quality & 0x80) | quantity;
		}
	}
}

unsigned char far Item_getQuality(objref *ref)
{
	int extra;

	if ((char)Item_hasQuality(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->quality;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 0;
}

unsigned char far Item_getCharges(objref *ref)
{
	if ((char)Item_hasQuality(ref)) {
		return Item_getQuality(ref);
	} else {
		return 1;
	}
}

void far Item_setQuality(objref *ref, char quality)
{
	int extra;

	if ((char)Item_hasQuality(ref)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->quality = quality;
		} else {
			ITEM(ref->off)->data.direct.quality = quality;
		}
	}
}

unsigned char far Item_getRegion(objref *ref)
{
	int kind;
	int extra;

	kind = GetLocationKind(GetItemZAndStuff(ref));
	if (kind <= 3)
		return LoadedRegions[kind];
	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_REGION)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			return EXTRA(extra)->region;
		}
		return ITEM(ref->off)->data.direct.quality;
	}
	return 255;
}

void far Item_setRegion(objref *ref, unsigned char region)
{
	int extra;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_REGION)) {
		if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_RECORDS)) {
			extra = ITEM(ref->off)->data.extra;
			EXTRA(extra)->region = region;
		} else {
			ITEM(ref->off)->data.direct.quality = region;
		}
	}
}

int far GetItemRecordCount(int classFlags)
{
	int count = classFlags & CLASS_RECORDS;

	if (!count)
		count++;
	return count;
}

void far AllocateItemRecords(objref *ref, int count)
{
	objref record;
	int i;

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

int far FindRegionSlot(Loc x, Loc y)
{
	unsigned char region;
	int i;

	region = GetRegionAt(x, y, CurrentMap);
	for (i = 0; i < 4; i++)
		if (LoadedRegions[i] == region)
			return i;
	return 255;
}

inline int GetContentsOf(objref *container)
{
	objref contents = GetContainedItem(container);

	return contents.off;
}

void far Item_linkIntoContainer(objref *ref, objref container)
{
	Item_setLocationKind(ref, LOCATION_CONTAINED);
	ITEM(ref->off)->next = GetContentsOf(&container);
	Item_setContents(&container, *ref);
	Item_setContainer(ref, container);
}

unsigned char far CreateItem(objref *ref, TypeFrame type, CellCoord x, CellCoord y, int z)
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

unsigned char far CreateItemInContainer(objref *ref, TypeFrame type, objref container)
{
	if (container.valid() && (char)(CLASS_FLAGS(ITEM(container.off)) & CLASS_CONTENTS)) {
		if (CreateItem(ref, type)) {
			return PlaceItemInContainer(ref, container);
		}
	}
	return 0;
}

int far *far Item_findLink(objref *ref)
{
	int far *link = 0;
	unsigned char kind;
	int x, y;

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

char far Item_spillContents(objref *ref)
{
	char contained;
	objref current, container;
	Coord x, y;
	int z;

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

char far Item_deleteContents(objref *ref)
{
	objref current;

	while ((current = GetContainedItem(ref)).valid())
		Item_delete(&current);
	return 0;
}

objref far Item_getContainer(objref *ref)
{
	objref parent = 0;

	if (IsContained(GetItemZAndStuff(ref)))
		parent.off = ITEM(ref->off)->position.parent;
	return parent;
}

void far Item_setContainer(objref *ref, objref parent)
{
	if (IsContained(GetItemZAndStuff(ref)))
		ITEM(ref->off)->position.parent = parent.off;
}

objref far GetContainedItem(objref *ref)
{
	objref contents = 0;
	int extra;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS)) {
		extra = ITEM(ref->off)->data.extra;
		contents.off = EXTRA(extra)->contents;
	}
	return contents;
}

int far Item_getExtraRecord(objref *ref)
{
	int extra = 0;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS))
		extra = ITEM(ref->off)->data.extra;
	return extra;
}

void far Item_setContents(objref *ref, objref contents)
{
	int extra;

	if ((char)(CLASS_FLAGS(ITEM(ref->off)) & CLASS_CONTENTS)) {
		extra = ITEM(ref->off)->data.extra;
		EXTRA(extra)->contents = contents.off;
	}
}

unsigned char far Item_isWithin(objref *ref, objref container)
{
	objref saved;
	unsigned char found = 0;

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

void far FreeItemRecord(objref *ref)
{
	int far *record = (int far *)ITEM(ref->off);

	*record = ItemFreeList;
	*++record = 0;
	*++record = 0;
	*++record = 0;
	ItemFreeList = ref->off;
	ItemFreeCount++;
}

char far Item_delete(objref *ref)
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

void far Item_setZ(objref *ref, int z)
{
	int x, y;

	if (GetItemZAndStuff(ref).z() != z) {
		if (IsOnMap(GetItemZAndStuff(ref))) {
			x = Item_getX(*ref).value;
			y = Item_getY(*ref).value;
			Item_detach(ref);
			MaskItemZAndStuff(ref, 0x0f);
			OrItemZAndStuff(ref, (unsigned char)z << 4);
			PlaceItem(ref, x, y);
		}
	}
}

unsigned char far Item_move(objref *ref, CellCoord x, CellCoord y)
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

unsigned char far Item_move(objref *ref, Loc x, Loc y, int z)
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

unsigned char far Item_forceMove(objref *ref, Loc x, Loc y, int z)
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

void far Item_move(objref *ref, unsigned char dir)
{
	MoveItem(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir]);
}

void far Item_move(objref *ref, unsigned char dir, int dz)
{
	MoveItem(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir],
		GetItemZAndStuff(ref).z() + dz);
}

void far Item_forceMove(objref *ref, unsigned char dir, int dz)
{
	MoveDirect(ref, Item_getX(*ref) + DirDeltaX[dir], Item_getY(*ref) + DirDeltaY[dir],
		GetItemZAndStuff(ref).z() + dz);
}

unsigned char far Item_moveIntoContainer(objref *ref, objref container)
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

unsigned char far Item_detach(objref *ref)
{
	int far *link;
	unsigned char notify;
	int kind;

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
		if ((unsigned char)(TYPE_FLAGS(ref) & CLASS_REGION) && kind <= 3)
			Item_setRegion(ref, LoadedRegions[kind]);
		if (notify) {
			OnStrangeMoverRemoved(*ref);
		}
		return 1;
	}
	return 0;
}

unsigned char far CreateItem(objref *ref, TypeFrame frame)
{
	objref empty;
	int flags, count;

	flags = ItemTypeClassFlags[gItemTypeInfo[frame.bits & 0x3ff].typeClass];
	count = GetItemRecordCount(flags);
	AllocateItemRecords(ref, count);
	if (ref->valid()) {
		ITEM(ref->off)->setTypeFrame(frame);
		Item_clearTemporary(ref);
		Item_setQualityFlags(ref, 0);
		if ((unsigned char)(TYPE_FLAGS(ref) & CLASS_CONTENTS)) {
			empty.off = 0;
			Item_setContents(ref, empty);
		}
		ITEM(ref->off)->next = DetachedItems.off;
		DetachedItems.off = ref->off;
		return 1;
	}
	return 0;
}

unsigned char far PlaceItemDirect(objref *ref, Loc x, Loc y, int z)
{
	int far *head;
	int chunkX, chunkY, regionIndex;

	ref->off = DetachedItems.off;
	if (ref->valid()) {
		MaskItemZAndStuff(ref, 15);
		OrItemZAndStuff(ref, (unsigned char)z << 4);
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
			if ((unsigned char)(TYPE_FLAGS(ref) & CLASS_REGION))
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

unsigned char far PlaceItem(objref *ref, Loc x, Loc y, int z)
{
	ref->off = DetachedItems.off;
	if (ref->valid()) {
		MaskItemZAndStuff(ref, 15);
		OrItemZAndStuff(ref, (unsigned char)z << 4);
		return PlaceItem(ref, x, y);
	}
	return 0;
}

unsigned char far PlaceItemInContainer(objref *ref, objref container)
{
	ref->off = DetachedItems.off;
	if (ref->valid()) {
		DetachedItems.off = ITEM(ref->off)->next;
		Item_linkIntoContainer(ref, container);
		return 1;
	}
	return 0;
}

unsigned char far PlaceItemOffMap(objref *ref)
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

unsigned char far ZapDetachedItem(objref *ref)
{
	objref npc;
	unsigned char records;
	unsigned char result = 0;
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

int far GetItemBeingDragged(objref *ref)
{
	ref->off = DetachedItems.off;
	if (ref->off != 0) {
		return 1;
	}
	return 0;
}

unsigned char far IsItemDetached(objref *ref)
{
	objref current = DetachedItems.off;

	while (IS_VALID(current.off)) {
		if (current == *ref)
			return 1;
		current = current.next();
	}
	return 0;
}

int far CountDetachedItems()
{
	int count = 0;

	for (objref ref = DetachedItems.off; ref.valid(); ref = ref.next())
		count++;
	return count;
}

extern "C" int far Item_greatestDeltaToItem(objref &ref, objref other)
{
	Coord x1, y1, x2, y2;
	unsigned z1, z2;

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

void far Item_setInvisible(objref *ref)
{
	Item_setQualityFlags(ref, Item_getQualityFlags(ref) | QUALITY_INVISIBLE);
	if ((char)((CLASS_FLAGS(ITEM(ref->off)) & CLASS_NPC) != 0))
		CombatGroups.loseSightOf(ref->off);
}

int far Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, unsigned targetZ)
{
	Coord x, y;

	Item_getXAndY(ref, &x.value, &y.value);
	unsigned z = Item_getZ(&ref);

	if (targetX < x)
		x = x - GetFootprintX(ITEM(ref.off)->asTypeFrame()) + 1;
	if (targetY < y)
		y = y - GetFootprintY(ITEM(ref.off)->asTypeFrame()) + 1;
	if (targetZ < z)
		z = z - gItemTypeInfo[TYPE(ITEM(ref.off))].height + 1;
	return GetDistance(x, y, z, targetX, targetY, targetZ);
}

unsigned char far Item_getDirToItem(objref *item, objref other, unsigned char cardinal)
{
	return DirectionTo(*item, Item_getX(other), Item_getY(other), cardinal);
}

unsigned char far GetDiagonalStep(int major, int minor)
{
	if ((minor << 1) <= major) {
		return 0;
	} else {
		if ((major << 1) <= minor)
			return 2;
	}
	return 1;
}

unsigned char far Item_getDirToCoords(objref *item, Coord x, Coord y, unsigned char cardinal)
{
	int dx = x.value - Item_getX(*item).value;
	int dy = y.value - Item_getY(*item).value;
	int absX = dx < 0 ? -dx : dx;
	int absY = dy < 0 ? -dy : dy;

	if (dx < 0) {
		if (dy < 0) {
			if (!cardinal)
				return ((int)GetDiagonalStep(absX, absY) + 6) & 7;
			else if (absX > absY)
				return 6;
			else
				return 0;
		} else if (dy > 0) {
			if (!cardinal)
				return (int)GetDiagonalStep(absY, absX) + 4;
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
				return (int)GetDiagonalStep(absY, absX);
			else if (absX > absY)
				return 2;
			else
				return 0;
		} else if (dy > 0) {
			if (!cardinal)
				return (int)GetDiagonalStep(absX, absY) + 2;
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

void far CheckItemHandle(unsigned handle)
{
	if ((handle & 7) != 0 || handle > ItemBufferBytes)
		FatalError("Item corrupt.\n");
}

void far CheckItemLists(void)
{
	objref current;
	int k, j, i;

	for (k = 0; k < 4; k++)
		for (j = 0; j < 16; j++)
			for (i = 0; i < 16; i++) {
				current = ChunkItemLists[k][j][i];
				while (IS_VALID(current.off)) {
					if ((current.off & 7) != 0 || current.off > ItemBufferBytes)
						FatalError("Item list corrupt.\n");
					current = current.next();
				}
			}
}

void far ReportBadFreeList(char *file, int line)
{
	FatalError("Bad Free List @%s, %d", file, line);
}

