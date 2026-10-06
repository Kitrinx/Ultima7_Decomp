/* Serpent Isle SI.EXE, resident segment 79 (file offsets 0x03199f to 0x03259c, 3069 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 */

#include "lowlevel.h"
#include "dosio.h"
#include "item.h"
#include "coord.h"
#include "voolook.h"
#include "u7npc.h"
#include "sortitem.h"
#include "oops.h"
#include "equip.h"
#include "itemovr1.h"
#include "makemojo.h"
#include "combat.h"
#include "wihh.h"
#include "chunk.h"
#include "mapview.h"
#include "monsters.h"
#include "type.h"
#include "itembuf.h"
#include "memapi.h"
#include "voonpc.h"
#include "ready.h"
#include "itemovr2.h"

#define TYPE(rec) ((rec)->typeFrame & 0x3ff)

/* one NPC in the save file: its item, its extra record and its NPC record */
struct far NpcSaveRecord {
	unsigned char x, y;
	unsigned typeFrame;
	ItemRecord extra;
	NpcBuffer actor;
};

struct MonsterRef {
	int index;
	MonsterRecord *operator->() { return MonsterRecords.get(index); }
};

extern int NpcItemRefs[];

inline unsigned char IsLocation(ItemInfo far &info, unsigned char kind) { return info.kind() == kind; }

void far ResetNpcRecords(objref *manager)
{
	int i;

	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++) {
		NpcItemRefs[i] = 0;
	}
	FreeNpcNumbers = 0;
	for (i = 0; i < NpcRecordCount - 1; i++) {
		GetCachedNpcBuffer(&NpcPool, i)->nextFree = i + 1;
	}
	GetCachedNpcBuffer(&NpcPool, NpcRecordCount - 1)->nextFree = -1;
	FreeMonsterNumbers = NpcRecordCount;
	for (i = 0; i < ExtraNpcRecordCount - 1; i++) {
		GetCachedNpcBuffer(&NpcPool, i + NpcRecordCount)->nextFree = i + NpcRecordCount + 1;
	}
	GetCachedNpcBuffer(&NpcPool, NpcRecordCount + ExtraNpcRecordCount - 1)->nextFree = -1;
}

void far AllocNpcRecords(objref *manager, int count, int extra)
{
	if (!InitNpcCache(&NpcPool, count + extra)) {
		ReportOutOfVoodooMemory();
	}
	ResetNpcRecords(manager);
}

void far RelinkNpcsInArea()
{
	objref actor;
	Coord x, y;
	int i, area;

	for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++) {
		GetNpcIbo(&actor, i);
		if (actor.valid() && TYPE(ITEM(actor.off)) != 0 && IsLocation(GetItemZAndStuff(&actor), LOCATION_OFF_MAP)) {
			x = Item_getX(actor);
			y = Item_getY(actor);
			area = FindRegionSlot(x, y);
			if (area != 255 && Item_detach(&actor)) {
				PlaceItem(&actor, x, y);
			}
		}
	}
}

void far RebuildNpcFreeLists(objref *actor)
{
	int i;

	FreeNpcNumbers = -1;
	for (i = NpcRecordCount - 1; i >= 0; i--) {
		actor->off = NpcItemRefs[i];
		if (!actor->valid()) {
			GetCachedNpcBuffer(&NpcPool, i)->nextFree = FreeNpcNumbers;
			FreeNpcNumbers = i;
		}
	}
	FreeMonsterNumbers = -1;
	for (i = ExtraNpcRecordCount - 1; i >= 0; i--) {
		actor->off = NpcItemRefs[i + NpcRecordCount];
		if (!actor->valid()) {
			GetCachedNpcBuffer(&NpcPool, i + NpcRecordCount)->nextFree = FreeMonsterNumbers;
			FreeMonsterNumbers = i + NpcRecordCount;
		}
	}
}

void far EquipCarriedItems(objref *actor)
{
	int number = Item_getNpcNumber(actor);
	objref current;
	unsigned char slot;

	if (number != -1) {
		current = GetContainedItem(actor);
		while (current.valid()) {
			slot = ReadyRecords.get(ReadyLookup.get(TYPE(ITEM(current.off))))->slot;
			if (!IsLocation(GetItemZAndStuff(&current), LOCATION_EQUIPPED)
				&& !objref(GetItemInSlot(*actor, slot)).valid()) {
				EquipItem(current, *actor, slot, 0);
			}
			current = current.next();
		}
	}
}

void far LoadNpcs(char *filename)
{
	int count, extraCount, extra;
	objref actor;
	MonsterRef type;
	int i, file;
	NpcSaveRecord data;
	int slots[18];

	for (i = 0; i < 18; i++)
		slots[i] = 0;
	file = DosOpen(filename);
	if (file >= 0) {
		if (DosRead(file, 0L, 2L, &count) != 2L)
			ReportInvalidSaveGame();
		if (DosRead(file, -1L, 2L, &extraCount) != 2L)
			ReportInvalidSaveGame();
		if (count > NpcRecordCount || extraCount > ExtraNpcRecordCount)
			ReportInvalidSaveGame();
		ResetNpcRecords(&actor);
		for (i = 0; i < count + extraCount; i++) {
			if (DosRead(file, -1L, sizeof(data), &data) != sizeof(data))
				ReportInvalidSaveGame();
			MoveFarMemory(GetCachedNpcBuffer(&NpcPool, i), &data.actor, sizeof(NpcBuffer));
			NpcItemRefs[i] = GetCachedNpcBuffer(&NpcPool, i)->ref;
			if (NpcItemRefs[i] != 0) {
				AllocateItemRecords(&actor, 2);
				NpcItemRefs[i] = actor.off;
				if (actor.valid()) {
					extra = ITEM(actor.off)->data.extra;
					MoveFarMemory(ITEM(extra), &data.extra, sizeof(data.extra));
					ITEM(actor.off)->position.world.x = data.x;
					ITEM(actor.off)->position.world.y = data.y;
					ITEM(actor.off)->setTypeFrame(data.typeFrame);
					GetNpcBufferForIbo(&actor)->setType(data.typeFrame);
					Item_setLocationKind(&actor, LOCATION_OFF_MAP);
					ITEM(actor.off)->next = 0;
					Item_setLocationKind(&actor, LOCATION_OFF_MAP);
					if (TYPE(ITEM(actor.off)) != 0 && GetContainedItem(&actor).valid()) {
						ReadItemTree(&actor, file, LOCATION_CONTAINED);
						GetNpcIbo(&actor, i);
					}
					/* clear its equipment slots, then fill them from what it carries */
					CopyFarToLinear(EquipList + i * sizeof(slots), slots, sizeof(slots));
					EquipCarriedItems(&actor);
					SelectWeapon(actor, 0, 0);
					type.index = MonsterLookup.get(TYPE(ITEM(actor.off)));
					GetNpcBufferForIbo(&actor)->typeFlags = (unsigned char)type->walk
						? GetNpcBufferForIbo(&actor)->typeFlags | NPC_WALK
						: GetNpcBufferForIbo(&actor)->typeFlags & ~NPC_WALK;
					GetNpcBufferForIbo(&actor)->typeFlags = (unsigned char)type->swim
						? GetNpcBufferForIbo(&actor)->typeFlags | NPC_SWIM
						: GetNpcBufferForIbo(&actor)->typeFlags & ~NPC_SWIM;
					GetNpcBufferForIbo(&actor)->typeFlags = (unsigned char)type->fly
						? GetNpcBufferForIbo(&actor)->typeFlags | NPC_FLY
						: GetNpcBufferForIbo(&actor)->typeFlags & ~NPC_FLY;
					/* an ethereal monster with a solid type flies instead */
					if ((unsigned char)type->ethereal) {
						if ((unsigned char)gItemTypeInfo[TYPE(ITEM(actor.off))].solid) {
							GetNpcBufferForIbo(&actor)->typeFlags = GetNpcBufferForIbo(&actor)->typeFlags | NPC_FLY;
						} else {
							GetNpcBufferForIbo(&actor)->typeFlags =
								GetNpcBufferForIbo(&actor)->typeFlags | NPC_ETHEREAL;
						}
					} else {
						GetNpcBufferForIbo(&actor)->typeFlags = GetNpcBufferForIbo(&actor)->typeFlags & ~NPC_ETHEREAL;
					}
				}
			}
		}
		DosClose(file);
		RebuildNpcFreeLists(&actor);
	}
}

void far SaveNpcs(char *filename)
{
	int extra;
	objref actor;
	int i, file;
	NpcSaveRecord data;

	file = DosCreate(filename);
	if (file >= 0) {
		DosWrite(file, 0L, 2L, &NpcRecordCount);
		DosWrite(file, -1L, 2L, &ExtraNpcRecordCount);
		for (i = 0; i < NpcRecordCount + ExtraNpcRecordCount; i++) {
			MoveFarMemory(&data.actor, GetCachedNpcBuffer(&NpcPool, i), sizeof(NpcBuffer));
			data.actor.ref = NpcItemRefs[i];
			if (NpcItemRefs[i] != 0) {
				actor.off = NpcItemRefs[i];
				data.x = ITEM(actor.off)->position.world.x;
				data.y = ITEM(actor.off)->position.world.y;
				data.typeFrame = ITEM(actor.off)->typeFrame;
				extra = ITEM(actor.off)->data.extra;
				MoveFarMemory(&data.extra, ITEM(extra), sizeof(data.extra));
			}
			DosWrite(file, -1L, sizeof(data), &data);
			if (NpcItemRefs[i] != 0) {
				actor.off = NpcItemRefs[i];
				actor = GetContainedItem(&actor);
				if (actor.valid()) {
					WriteItemTree(&actor, file);
				}
			}
		}
		DosClose(file);
	} else {
		ReportInvalidSaveGame();
	}
}

void far ResetObjectLists()
{
	ItemRenderOrder.front = ItemRenderOrder.begin;
	ItemRenderOrder.back = ItemRenderOrder.end;
}

void far InitItemManager(WorldView *view)
{
	view->cache.initialize();
	CellBuffer = (CellRow far *)AllocateFarHeap(CELL_WINDOW * sizeof(CellRow), 0);
	if (!CellBuffer) {
		ReportOutOfFarMemory();
	}
	view->terrain = (TerrainRegion far *) AllocateFarHeap(4 * sizeof(TerrainRegion), 0);
	if (!view->terrain) {
		ReportOutOfFarMemory();
	}
}
