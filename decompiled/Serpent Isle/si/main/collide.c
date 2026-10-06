/* Serpent Isle SI.EXE, resident segment 10 (file offsets 0x00f3f3 to 0x010af0, 5885 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: collide.c */
#include <dos.h>
#include "typefram.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "objref.h"
#include "u7npc.h"
#include "vooalloc.h"
#include "collgrid.h"
#include "u7sound.h"
#include "oops.h"
#include "sounds.h"
#include "weight.h"
#include "random.h"
#include "actqueue.h"
#include "daze.h"
#include "script.h"
#include "type.h"
#include "item.h"
#include "npcref.h"
#include "coord.h"
#include "mapview.h"
#include "search.h"
#include "collide.h"

#define ITEM(ref) ((ItemRecord far *)MK_FP(ItemBufferSegment, (ref).off))
#define TYPE(s) ((s).bits & 0x3ff)
#define FRAME(s) (((s).bits & 0x7c00) >> 10)
#define TYPE_CLASS(s) (gItemTypeInfo[TYPE(s)].typeClass)
#define IS_CLASS(s, n) ((unsigned char)(TYPE_CLASS(s) == (n)))
#define IS_NPC(s) ((unsigned char)((ItemTypeClassFlags[TYPE_CLASS(s)] & CLASS_NPC) != 0))
#define IS_SET(f, mask) ((char)(((f) & (mask)) != 0))
#define IS_VALID(r) ((char)((r) != 0))
#define IS_SOLID(s) ((char) gItemTypeInfo[TYPE(s)].solid)
#define IS_CONTACT(s) ((char) gItemTypeInfo[TYPE(s)].field)
#define IS_DOOR(s) ((char) gItemTypeInfo[TYPE(s)].door)

/* the collision grid covers this many cells each way around the cell window */
#define COLLISION_CELLS 80

inline void ChangeNpcFlags(objref &npc, unsigned clear, unsigned set) { NPCRef(npc).changeFlags(clear, set); }

extern long CollisionGrid;

unsigned char BlockedByDoor = 0;
unsigned char InDungeon = 0;
long HeightMasks[16] = {
	0x0L, 0x1L, 0x5L, 0x15L, 0x55L, 0x155L, 0x555L, 0x1555L,
	0x5555L, 0x15555L, 0x55555L, 0x155555L, 0x555555L, 0x1555555L, 0x5555555L, 0x15555555L
};

void far AllocateCollisionBuffer()
{
	if ((CollisionGrid = AllocateVoodooMemory(&VoodooXmsBlock, 0xa000L)) == 0)
		ReportOutOfVoodooMemory();
}

char far WorldToCollisionCell(CellCoord x, CellCoord y, int *dx, int *dy, int margin)
{
	unsigned limit;

	*dx = x - CellWindowX;
	*dy = y - CellWindowY;
	limit = margin + COLLISION_CELLS;
	return *dx < limit && *dy < limit;
}

unsigned char far IsTypeBlockedAt(CellCoord x, CellCoord y, int z, TypeFrame typeFrame)
{
	int dx, dy, w, h;
	long mask, hits;

	BlockedByDoor = 0;
	if (WorldToCollisionCell(x, y, &dx, &dy, 0)) {
		w = GetFootprintX(typeFrame);
		h = GetFootprintY(typeFrame);
		if (w > dx || h > dy)
			return 1;
		mask = HeightMasks[gItemTypeInfo[TYPE(typeFrame)].height] << (z * 2);
		mask |= mask << 1;
		hits = OrCollisionBlock(dy, dx, h, w) & mask;
		if (hits & 0xaaaaaaaaL)
			BlockedByDoor = 1;
		return (hits & 0x55555555L) != 0;
	}
	return 1;
}

unsigned char far IsBoxBlockedAt(CellCoord x, CellCoord y, int z, int w, int h, int height)
{
	int dx, dy;
	long mask, hits;

	w--;
	h--;
	BlockedByDoor = 0;
	if (WorldToCollisionCell(x, y, &dx, &dy, 0) != 0) {
		if (w > dx || h > dy)
			return 1;
		mask = HeightMasks[height] << (z * 2);
		mask |= mask << 1;
		hits = OrCollisionBlock(dy, dx, h, w) & mask;
		if (hits & 0xaaaaaaaaL)
			BlockedByDoor = 1;
		return (hits & 0x55555555L) != 0;
	}
	return 1;
}

unsigned char far IsTypeSupportedAt(CellCoord x, CellCoord y, int z, TypeFrame typeFrame)
{
	int dx, dy, w, h;
	long bit;

	if (WorldToCollisionCell(x, y, &dx, &dy, 0)) {
		if (z == 0)
			return 1;
		w = GetFootprintX(typeFrame);
		h = GetFootprintY(typeFrame);
		bit = 1L << ((z - 1) * 2);
		return IsCollisionBlockSet(dy, dx, h, w, bit);
	}
	return 0;
}

int far FindSupportLevel(CellCoord x, CellCoord y, int z, TypeFrame far *typeFrame)
{
	for (; z != 0; z--) {
		if (IsTypeSupportedAt(x + z, y + z, z, *typeFrame))
			break;
	}
	return z;
}

void far AddTypeToCollision(CellCoord x, CellCoord y, int z, TypeFrame typeFrame)
{
	int dx, dy, left, w, h;
	long mask;

	if (WorldToCollisionCell(x, y, &dx, &dy, 7)) {
		left = dx;
		h = GetFootprintY(typeFrame);
		if (dy >= COLLISION_CELLS) {
			h -= dy - (COLLISION_CELLS - 1);
			dy = COLLISION_CELLS - 1;
		}
		mask = HeightMasks[gItemTypeInfo[TYPE(typeFrame)].height] << (z * 2);
		if (IS_DOOR(typeFrame))
			mask <<= 1;
		for (; dy >= 0 && h >= 0; dy--, h--) {
			dx = left;
			w = GetFootprintX(typeFrame);
			if (dx >= COLLISION_CELLS) {
				w -= dx - (COLLISION_CELLS - 1);
				dx = COLLISION_CELLS - 1;
			}
			for (; dx >= 0 && w >= 0; dx--, w--)
				OrCollisionCell(dy, dx, mask);
		}
	}
}

void far AddTypeToCollision(objref item)
{
	TypeFrame typeFrame = ITEM(item)->typeFrame;

	if (IS_SOLID(typeFrame))
		AddTypeToCollision(Item_getX(item), Item_getY(item), Item_getZ(&item), typeFrame);
}

void far RemoveTypeFromCollision(CellCoord x, CellCoord y, int z, TypeFrame typeFrame)
{
	int dx, dy, left, w, h;
	long mask;

	if (WorldToCollisionCell(x, y, &dx, &dy, 7)) {
		left = dx;
		h = GetFootprintY(typeFrame);
		if (dy >= COLLISION_CELLS) {
			h -= dy - (COLLISION_CELLS - 1);
			dy = COLLISION_CELLS - 1;
		}
		mask = HeightMasks[gItemTypeInfo[TYPE(typeFrame)].height] << (z * 2);
		if (IS_DOOR(typeFrame))
			mask <<= 1;
		mask = ~mask;
		for (; dy >= 0 && h >= 0; dy--, h--) {
			dx = left;
			w = GetFootprintX(typeFrame);
			if (dx >= COLLISION_CELLS) {
				w -= dx - (COLLISION_CELLS - 1);
				dx = COLLISION_CELLS - 1;
			}
			for (; dx >= 0 && w >= 0; dx--, w--)
				AndCollisionCell(dy, dx, mask);
		}
	}
}

void far RemoveTypeFromCollision(objref item)
{
	TypeFrame typeFrame = ITEM(item)->typeFrame;

	if (IS_SOLID(typeFrame))
		RemoveTypeFromCollision(Item_getX(item), Item_getY(item), Item_getZ(&item), typeFrame);
}

void far ClearCollisionBuffer()
{
	int y, x;

	for (y = 0; y < COLLISION_CELLS; y++)
		for (x = 0; x < COLLISION_CELLS; x++)
			SetCollisionCell(y, x, 0L);
}

void far AddChunkToCollision(CellCoord x, CellCoord y)
{
	AreaSearch found;

	FindItemInArea(&found, x, y, (x + 16) - 1, (y + 16) - 1, 0xb0, -1, -1, 255);
	while (found.current.valid()) {
		AddTypeToCollision(found.current);
		FindItem(&found);
	}
}

void far UpdateCeiling(ItemId id)
{
	int dx, dy, h, w;
	long mask;
	TypeFrame typeFrame;
	objref item = id.off;
	int ceiling = 15;

	if (WorldToCollisionCell(Item_getX(item), Item_getY(item), &dx, &dy, 0)) {
		typeFrame = ITEM(item)->typeFrame;
		w = GetFootprintX(typeFrame);
		h = GetFootprintY(typeFrame);
		mask = 0;
		for (; dy >= 0 && h >= 0; dy--, h--)
			for (; dx >= 0 && w >= 0; dx--, w--)
				mask |= GetCollisionCell(dy, dx);
		mask &= ~HeightMasks[gItemTypeInfo[TYPE(typeFrame)].height] << (Item_getZ(&item) * 2);
		mask &= 0x55555555L;
		while (mask != 0 && ceiling > 0) {
			ceiling--;
			mask <<= 2;
		}
	}
	CeilingZ = ceiling;
	if (CeilingZ < 15)
		UpdateDungeonState(id);
	else if (InDungeon)
		SetInDungeon(0);
}

unsigned char far IsPointInItem(ItemId id, Coord x, Coord y, int z)
{
	int dx, dy, dz;
	objref item = id.off;
	TypeFrame typeFrame = ITEM(item)->typeFrame;

	dx = Item_getX(item) - x;
	dy = Item_getY(item) - y;
	dz = z - Item_getZ(&item);
	return dx >= 0 && GetFootprintX(typeFrame) >= dx
		&& dy >= 0 && GetFootprintY(typeFrame) >= dy
		&& dz >= 0 && gItemTypeInfo[TYPE(typeFrame)].height > dz;
}

void far SetInDungeon(unsigned char inside)
{
	if (inside != 0 && inside != InDungeon) {
		InDungeon = inside;
		PlayMusic(42);
	}
	if (!inside && InDungeon != 0) {
		InDungeon = 0;
		PlayMusic(67);
	}
}

unsigned char far IsUnderMountain(ItemId id)
{
	unsigned char found = 0;

	if (IS_VALID(id.off)) {
		objref p, item = id.off;
		TypeFrame typeFrame;
		unsigned char chunk;
		int cx, cy;
		unsigned z;
		int left, top, x, y;
		ItemInfo info;
		int type;

		info = GetItemZAndStuff(&item);
		chunk = info.kind();
		if (chunk <= 3) {
			typeFrame = ITEM(item)->typeFrame;
			x = ITEM(item)->position.world.x;
			left = x - GetFootprintX(typeFrame);
			y = ITEM(item)->position.world.y;
			top = y - GetFootprintY(typeFrame);
			cx = x >> 4;
			cy = y >> 4;
			z = (unsigned char)(Item_getZ(&item) + gItemTypeInfo[TYPE(ITEM(item)->asTypeFrame())].height);
			p = ChunkItemLists[chunk][cy][cx];
			while (p.valid()) {
				typeFrame = ITEM(p)->typeFrame;
				type = TYPE(typeFrame);
				/* mountain */
				if (type == 969 || type == 983 || type == 180 || type == 182 || type == 183 || type == 324
					|| type == 941) {
					cx = ITEM(p)->position.world.x;
					cy = ITEM(p)->position.world.y;
					if (cx >= left && cx - GetFootprintX(typeFrame) <= x
						&& cy >= top && cy - GetFootprintY(typeFrame) <= y
						&& Item_getZ(&p) >= z) {
						found = 1;
						break;
					}
				} else if (type == 436 || type == 437 || type == 444 || type == 448 || type == 466 || type == 477) {
					cx = ITEM(p)->position.world.x;
					cy = ITEM(p)->position.world.y;
					if (cx >= left && cx - GetFootprintX(typeFrame) <= x
						&& cy >= top && cy - GetFootprintY(typeFrame) <= y
						&& Item_getZ(&p) >= z) {
						found = 2;
						break;
					}
				} else if (type == 394) {
					cx = ITEM(p)->position.world.x;
					cy = ITEM(p)->position.world.y;
					if (cx >= left && cx - GetFootprintX(typeFrame) <= x
						&& cy >= top && cy - GetFootprintY(typeFrame) <= y
						&& Item_getZ(&p) >= z) {
						found = 3;
						break;
					}
				}
				p = p.next();
			}
		}
	}
	return found;
}

void far UpdateDungeonState(ItemId id)
{
	SetInDungeon(IsUnderMountain(id));
}

void far SettleChunkItems(CellCoord x, CellCoord y)
{
	unsigned char dropped = 0;
	objref p, first;
	TypeFrame typeFrame;
	int cx, cy, slot, z, w, h;

	slot = FindRegionSlot(x.value, y.value);
	if (slot == 255)
		return;
	x.value = x.value & 0xfff0;
	x.value = (x.value + 3072) % 3072;
	y.value = y.value & 0xfff0;
	y.value = (y.value + 3072) % 3072;
	AddChunkToCollision(x, y);
	cx = (x.value & 0xff) >> 4;
	cy = (y.value & 0xff) >> 4;
	p.off = first.off = ChunkItemLists[slot][cy][cx];
	while (p.valid()) {
		typeFrame = ITEM(p)->typeFrame;
		if ((GetItemTypeWeight(TYPE(typeFrame)) != 0 || IS_NPC(ITEM(p)->asTypeFrame()))
			&& !(Item_getQualityFlags(&p) & 0x10)
			&& !IS_CLASS(ITEM(p)->asTypeFrame(), TYPE_CLASS_BUILDING)
			&& (!IS_NPC(ITEM(p)->asTypeFrame()) || !(GetNpcBufferForIbo(&NPCRef(p))->typeFlags & NPC_FLY))) {
			x.value = Item_getX(p);
			y.value = Item_getY(p);
			z = Item_getZ(&p);
			w = GetFootprintX(typeFrame) + 1;
			h = GetFootprintY(typeFrame) + 1;
			while (z > 0 && !IsBoxBlockedAt(x, y, z - 1, w, h, 1))
				z--;
			if (Item_getZ(&p) != z) {
				Item_move(&p, x.value, y.value, z);
				if (!dropped) {
					dropped = 1;
					PlaySoundAtItem(80, p);
				}
				p = first;
				continue;
			}
		}
		p = p.next();
	}
}

void far ApplyContactEffect(objref npc, objref item)
{
	unsigned char both = 0;
	unsigned type = TYPE(ITEM(item)->asTypeFrame());
	unsigned char hit;

	if (!(Item_getQualityFlags(&item) & QUALITY_BUSY)) {
		hit = 0;
		switch (type) {
		case 825:   /* campfire */
			if (FRAME(ITEM(item)->asTypeFrame()) == 8)
				break;
			ActionQueue.add(npc.off, MakeScript(SCRIPT_NO_HALT, SCRIPT_HIT, GenerateRandomIntegerInRange(2) + 2,
				1, SCRIPT_END));
			hit = 1;
			break;
		case 561:
		case 895:   /* fire field */
			ActionQueue.add(npc.off, MakeScript(SCRIPT_NO_HALT, SCRIPT_HIT, GenerateRandomIntegerInRange(2) + 2,
				1, SCRIPT_END));
			hit = 1;
			break;
		case 900:   /* poison field */
			if (FRAME(ITEM(item)->asTypeFrame()) >= 4 && ApplyPoison(npc)) {
				both = 1;
				hit = 1;
			}
			break;
		case 902:   /* sleep field */
			if (FRAME(ITEM(item)->asTypeFrame()) >= 6 && PutToSleep(npc)) {
				PutToSleep(npc);
				both = 1;
				hit = 1;
			}
			break;
		case 756:   /* caltrops */
			ActionQueue.add(npc.off, MakeScript(SCRIPT_NO_HALT, SCRIPT_HIT, GenerateRandomIntegerInRange(1) + 1,
				0, SCRIPT_END));
			hit = 1;
			break;
		}
		/* status 0x400 marks the npc as hurt by contact this step */
		if (hit)
			ChangeNpcFlags(npc, 0, 0x400);
		if (both)
			ActionQueue.add(item.off, MakeScript(SCRIPT_REMOVE, SCRIPT_PREV_FRAME_MIN, SCRIPT_CONTINUE,
				SCRIPT_PREV_FRAME_MIN, SCRIPT_LOOP, -3, FRAME(ITEM(item)->asTypeFrame()), SCRIPT_END));
	}
}

void far CheckContactEffects(objref npc)
{
	Coord cx, cy, left, top, x, y, px, py;
	int i, j, cxi, cyi, z, topZ;
	objref p;
	TypeFrame typeFrame, other;
	unsigned char chunk;
	int itemZ;

	ChangeNpcFlags(npc, 0x400, 0);
	x = Item_getX(npc);
	y = Item_getY(npc);
	z = Item_getZ(&npc);
	typeFrame = ITEM(npc)->typeFrame;
	left = x - GetFootprintX(typeFrame);
	top = y - GetFootprintY(typeFrame);
	topZ = gItemTypeInfo[TYPE(typeFrame)].height + z - 1;
	for (i = 0; i < 2; i++) {
		for (j = 0; j < 2; j++) {
			cx = x + i * 16;
			cy = y + i * 16;    /* i, not j, as shipped */
			chunk = FindRegionSlot(cx, cy);
			if (chunk != 255) {
				cxi = (cx & 0xff) >> 4;
				cyi = (cy & 0xff) >> 4;
				for (p = ChunkItemLists[chunk][cyi][cxi]; p.valid(); p = p.next()) {
					if (IS_CONTACT(ITEM(p)->asTypeFrame())) {
						px = Item_getX(p);
						py = Item_getY(p);
						itemZ = Item_getZ(&p);
						other = ITEM(p)->typeFrame;
						if (px >= left && px - GetFootprintX(other) <= x
							&& py >= top && py - GetFootprintY(other) <= y
							&& gItemTypeInfo[TYPE(other)].height + itemZ - 1 >= z
							&& itemZ <= topZ) {
							ApplyContactEffect(npc, p);
							break;
						}
					}
				}
			}
			if (IS_SET(GetNpcBufferForIbo(&NPCRef(npc))->status, 0x400))
				break;
		}
		if (IS_SET(GetNpcBufferForIbo(&NPCRef(npc))->status, 0x400))
			return;
	}
}
