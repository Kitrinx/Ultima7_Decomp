/* Serpent Isle SI.EXE, overlay segment 250 (file offsets 0x06cad0 to 0x06d630, 2912 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Y rebuilds it byte for byte as C++.
 */

/* path: vitem.c */
#include "u7port.h"
#include <new>
#include "plat.h"
#include "lowlevel.h"
#include "objref.h"
#include "itemrec.h"
#include "iteminfo.h"
#include "activity.h"
#include "coord.h"
#include "type.h"
#include "u7npc.h"
#include "init.h"
#include "flexvoo.h"
#include "debug.h"
#include "loadreg.h"
#include "uccomm5.h"
#include "easyfile.h"
#include "collide.h"
#include "search.h"
#include "mapview.h"
#include "maps.h"
#include "eggspawn.h"

#define ITEM(r) ((struct ItemRecord *) ItemAt((r).off))
#define TYPE(p) ((p)->typeFrame & 0x3ff)
#define TYPE_CLASS(p) (gItemTypeInfo[TYPE(p)].typeClass)
#define IS_CLASS(r, c) ((int8_t) (TYPE_CLASS(ITEM(r)) == (c)))
#define IS_VALID(n) ((int8_t) ((n) != 0))
#define NPC(r) GetNpcBufferForIbo(r)
#define CUR_SCHED(r) (NPC(r)->schedules[NPC(r)->currentSchedule])
#define IS_NPC(r) ((int8_t) ((ItemTypeClassFlags[TYPE_CLASS(ITEM(r))] & CLASS_NPC) != 0))

/* How far a lies past b, counting forward round the map. */
inline Coord ForwardDelta(const Coord &a, const Coord &b)
{
	return Coord(a.value - b.value);
}

/* One fixed object in a chunk's U7IFIX record: cell x and y, z, type and frame. */
struct IfixEntry {
	uint8_t xy;
	uint8_t z;
	uint16_t typeFrame;
};

extern int16_t ChunkItemLists[4][16][16];

/* One of four cached U7IFIX region files, reused oldest first. */
struct IfixCache : VoodooFlex {
	uint8_t chunk;
	uint8_t age;
	IfixCache() { chunk = 255; age = 255; }
	void markUsed();
};

extern int16_t FindRegionSlot(Loc, Loc);
extern objref GetContainedItem(objref *);
extern int8_t Item_spillContents(objref *ref);
extern int8_t Item_delete(objref *ref);
extern "C" void Egg_activate(objref, uint8_t);
extern uint8_t Item_getQuality(objref *ref);
extern void Item_setQuality(objref *, int8_t);
extern uint8_t CreateItem(objref *, TypeFrame);
extern uint8_t CreateItem(objref *, TypeFrame, CellCoord, CellCoord, int16_t);

char *IfixFileFormat = "u7ifix%02x";
char *IfixFilePattern = "u7ifix*.";
IfixCache IfixCaches[4];

/* Makes this the most recently used cache, ageing the ones used since. */
void IfixCache::markUsed()
{
	int16_t i;

	if (age != 0) {
		for (i = 0; i < 4; i++)
			if (IfixCaches[i].age < age)
				IfixCaches[i].age++;
		age = 0;
	}
}

/* The size of the largest U7IFIX file. */
int32_t GetLargestIfixSize(void)
{
	uint8_t done;
	int32_t max;
	plat_find ff;

	max = 0;
	done = !plat_find_first(BuildPath(StaticPath, IfixFilePattern, 0), &ff);
	while (!done) {
		if (ff.size > max)
			max = ff.size;
		done = !plat_find_next(&ff);
	}
	return max;
}

void InitIfixCaches(void)
{
	int32_t size;
	int16_t i;

	size = GetLargestIfixSize();
	for (i = 0; i < 4; i++) {
		IfixCaches[i].alloc(size);
		IfixCaches[i].age = i;
	}
}

/* The cache holding U7IFIX file n, loading it over the oldest one if none does. */
IfixCache *GetIfixCache(uint8_t n)
{
	IfixCache *cache = 0;
	int16_t i;

	for (i = 0; i < 4; i++) {
		if (IfixCaches[i].chunk == n) {
			IfixCaches[i].markUsed();
			return &IfixCaches[i];
		}
		if ((int16_t) IfixCaches[i].age == 3)
			cache = &IfixCaches[i];
	}
	if (cache == 0)
		AssertFail(__FILE__, 140);
	cache->load(BuildNumberedPath(StaticPath, IfixFileFormat, n, 0));
	cache->chunk = n;
	cache->markUsed();
	return cache;
}

/* Brings the chunk at x, y into the world: hatches its eggs and returns hostile NPCs to combat,
 * then creates the items drawn in its terrain and its fixed objects. */
void LoadChunkItems(Coord x, Coord y, WorldView *map)
{
	int16_t cx, cy, idx, count, slot;
	uint16_t type;
	objref ref, npc;
	uint8_t region, alignment;
	IfixEntry *base;
	int16_t i;

	if (ReloadingTerrain)
		return;
	slot = FindRegionSlot(x, y);
	cx = (x & 0xff) >> 4;
	cy = (y & 0xff) >> 4;
	if (slot != 255)
		for (ref = ChunkItemLists[slot][cy][cx]; IS_VALID(ref.off); ref = ref.next()) {
			if (IS_CLASS(ref, TYPE_CLASS_EGG) && RemoteViewActive == 0) {    /* an egg */
				objref egg = ref;

				if ((int8_t)Egg_rollChance((EggRecord *) ItemAt(ITEM(egg)->data.extra)))
					Egg_activate(egg, 0);
			}
			if (IS_NPC(ref)) {
				npc = ref;
				if (!IsDead(&npc) && NPC(&npc)->workType == WORK_COMBAT) {
					/* evil or chaotic */
					if ((alignment = (uint8_t) ((NPC(&npc)->status & 0x18) >> 3)) == 2 || alignment == 3) {
						uint8_t oldMode = Item_getQuality(&npc);

						Npc_setSchedule(&npc, WORK_COMBAT);
						CUR_SCHED(&npc).state = 1;
						if (oldMode == 7)
							Item_setQuality(&npc, oldMode);
					}
				}
			}
		}
	{
		int32_t addr = map->cache.getChunk(map->getChunkAt(x, y));

		for (i = 0; i < 256; i++) {
			type = TypeFrame(PeekWord(addr)).type();
			if (type >= 150 && gItemTypeInfo[type].typeClass == TYPE_CLASS_NONE)
				CreateItem(&ref, TypeFrame(PeekWord(addr)), Coord(x + (i & 0xf)), Coord(y + (i >> 4)), 0);
			addr += 2;
		}
	}
	base = (IfixEntry *)FileTransferBuffer;
	region = GetRegionAt(x, y, CurrentMap);
	{
		FlexEntry entry;
		IfixEntry *fixed;
		IfixCache *cache = GetIfixCache(region);

		if (cache == 0)
			return;
		idx = cx + cy * 16;
		if (cache->getEntry(idx, &entry)) {
			count = entry.size / 4;
			cache->readRecord(idx, base, 0);
			fixed = base;
			for (i = 0; i < count; i++) {
				CreateItem(&ref, TypeFrame(fixed->typeFrame).bits, (fixed->xy >> 4) + x, (fixed->xy & 0xf) + y,
					fixed->z);
				fixed++;
			}
		}
	}
}

/* Brings in the chunks of the window at nx, ny that the old window at ox, oy did not hold. */
void LoadWindowChunks(Coord ox, Coord oy, Coord nx, Coord ny, WorldView *view)
{
	int16_t j;
	int16_t dx, dy;
	Coord x, y;
	int16_t i;

	y = ny;
	for (j = 0; j < 96; j += 16) {
		dy = ForwardDelta(y, oy);
		x = nx;
		for (i = 0; i < 96; i += 16) {
			if (i < 80 && j < 80) {
				dx = ForwardDelta(x, ox);
				if (RegionsChanged || dx >= 80 || dy >= 80)
					LoadChunkItems(x, y, view);
			}
			AddChunkToCollision(x, y);
			x += 16;
		}
		y += 16;
	}
	ReloadingTerrain = 0;
}

/* Deletes the container's contents that bit 3 marks, spilling nested containers first. */
void RemoveContents(objref container)
{
	objref ref;
	int8_t again;
	AreaSearch search;

	again = 0;
	FindItemInContainer(&search, container, 0xb0, -1, 255, 255);
	while (search.found()) {
		ref.off = 0;
		if ((uint8_t)(GetItemZAndStuff(&search.current).flags & 8))
			ref = search.current;
		FindItem(&search);
		if (IS_VALID(ref.off)) {
			if (IS_VALID(GetContainedItem(&ref).off)) {
				again = 1;
				Item_spillContents(&ref);
			}
			Item_delete(&ref);
		}
		if (!search.found() && again) {
			again = 0;
			FindItemInContainer(&search, container, 0xb0, -1, 255, 255);
		}
	}
}

/* Takes the chunk at x, y out of the world. */
void UnloadChunk(Coord x, Coord y)
{
	objref item, cur;
	int16_t slot, cy, cx;

	if (!ReloadingTerrain) {
		slot = FindRegionSlot(x, y);
		cx = (x & 0xff) >> 4;
		cy = (y & 0xff) >> 4;
		if (slot == 255)
			return;
		item = ChunkItemLists[slot][cy][cx];
		while (IS_VALID(item.off)) {
			cur = item;
			item = item.next();
			if (!(IS_CLASS(cur, TYPE_CLASS_NONE) || IS_CLASS(cur, TYPE_CLASS_BUILDING)
				|| (uint8_t)(GetItemZAndStuff(&cur).flags & 8))) {
				if (IS_CLASS(cur, TYPE_CLASS_EGG))
					Egg_clearHatched(&objref(cur.off));
				DetachItem(cur.off);
				cur.off = 0;
			}
			if (IS_VALID(cur.off)) {
				if (IS_VALID(GetContainedItem(&cur).off))
					RemoveContents(cur);
				Item_delete(&cur);
			}
		}
	}
}

/* Drops the chunks of the old window at ox, oy that the new window at nx, ny leaves behind. */
void UnloadWindowChunks(Coord ox, Coord oy, Coord nx, Coord ny)
{
	int16_t j;
	int16_t dx, dy;
	Coord x, y;
	int16_t i;

	y = oy;
	for (j = 0; j < 80; j += 16) {
		dy = ForwardDelta(y, ny);
		x = ox;
		for (i = 0; i < 80; i += 16) {
			dx = ForwardDelta(x, nx);
			if (RegionsChanged || dx >= 80 || dy >= 80)
				UnloadChunk(x, y);
			x += 16;
		}
		y += 16;
	}
	ClearCollisionBuffer();
}

extern "C" void ResetVitemGlobals(void)
{
	IfixFileFormat = "u7ifix%02x";
	IfixFilePattern = "u7ifix*.";
	memset((void *)IfixCaches, 0, sizeof(IfixCaches));
}

extern "C" void ConstructVitemGlobals(void)
{
	int16_t i;

	for (i = 0; i < 4; i++)
		new (&IfixCaches[i]) IfixCache();
}
