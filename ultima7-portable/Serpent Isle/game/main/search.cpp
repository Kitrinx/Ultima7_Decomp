/* Serpent Isle SI.EXE, resident segment 29 (file offsets 0x018e32 to 0x01972c, 2298 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "iteminfo.h"
#include "itemrec.h"
#include "u7npc.h"
#include "type.h"
#include "search.h"
#include "objref.h"
#include "coord.h"
#include "item.h"

#define IS_VALID(r) ((uint8_t) ((r) != 0))
#define FLAG(v, m) ((uint8_t) ((v) & (m)))
#define KIND(b) ((uint8_t) ((int16_t) (b) & 7))
#define CLASS_FLAGS(s) (ItemTypeClassFlags[gItemTypeInfo[(s).type()].typeClass])
#define HAS_FLAG(s, f) ((uint8_t) ((CLASS_FLAGS(s) & (f)) != 0))
#define NPC_FLAG(r, f) ((uint8_t) ((GetNpcBufferForIbo(r)->status & (f)) != 0))

inline int16_t HasQuality(objref *r) { return (uint8_t)Item_hasQuality(r); }

inline uint8_t FoundEquipped(AreaSearch *p)
{
	ItemInfo z = GetItemZAndStuff(&p->current);
	return KIND(z.flags) == LOCATION_EQUIPPED;
}

/* The z of the item found. */
inline uint8_t FoundZ(AreaSearch *p)
{
	ItemInfo z = GetItemZAndStuff(&p->current);
	return (uint8_t) (z.flags & 0xf0) >> 4;
}

int8_t FindItemInArea(AreaSearch *search, Loc x, Loc y, int16_t flags, int16_t type, int8_t quality, int16_t frame)
{
	search->firstStep = 1;
	search->container.off = 0;
	search->start(x, y, x, y, 15, 0);
	search->flags = flags;
	search->type = type;
	search->quality = quality;
	search->frame = frame;
	return FindItem(search);
}

int8_t FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int16_t flags, int16_t type, int8_t quality,
	int16_t frame)
{
	search->firstStep = 1;
	search->container.off = 0;
	search->start(x0, y0, x1, y1, 15, 0);
	search->flags = flags;
	search->type = type;
	search->quality = quality;
	search->frame = frame;
	return FindItem(search);
}

int8_t FindItemInContainer(AreaSearch *search, objref container, int16_t flags, int16_t type, int8_t quality, int16_t frame)
{
	search->firstStep = 1;
	search->container = container.off;
	search->flags = flags;
	search->type = type;
	search->quality = quality;
	search->frame = frame;
	search->current.off = Item_getExtraRecord(&search->container);
	return FindItem(search);
}

/* Steps to the next item in the container, descending into containers and climbing back out. */
int16_t StepContainerSearch(AreaSearch *search)
{
	if ((search->flags & 0x100) || search->firstStep != 0) {
		search->current.off = *(int16_t *) ITEM(search->current.off);
	} else {
		int16_t current = search->current.off;
		objref child = GetContainedItem(&search->current);

		search->current.off = child.off;
		if (!IS_VALID(search->current.off))
			search->current.off = *(int16_t *) ITEM(current);
		while (!IS_VALID(search->current.off) && IsContained((objref *)&current) &&
			Item_getContainer((objref *)&current) != search->container.off) {
			objref outer = Item_getContainer((objref *)&current);

			current = outer.off;
			search->current.off = *(int16_t *) ITEM(current);
		}
	}
	search->firstStep = 0;
	return search->current.off != 0;
}

void StepAreaSearch(AreaSearch *search) { search->next(); }

/* Steps the search to its next match. Flags: 1 only equipped items, 2 none equipped, 4 only NPCs,
 * 8 only NPCs not dead, 0x10 include types of class flag 0x400, 0x20 include items whose quality
 * flag 1 is set (0x40 for party members), 0x80 include transparent types, 0x100 stay on a
 * container's top level. */
int8_t FindItem(AreaSearch *search)
{
	objref ref;
	TypeFrame typeFrame;

	for (;;) {
		if (IS_VALID(search->container.off))
			StepContainerSearch(search);
		else
			StepAreaSearch(search);
		if (!IS_VALID(search->current.off))
			return 0;
		ref = search->current.off;
		typeFrame = ITEM(search->current.off)->typeFrame;
		if (search->type != -1 && (uint16_t)(search->type) != typeFrame.type())
			continue;
		/* A byte compare, as Borland made it, so qualities over 127 match. */
		if (search->quality != (int8_t) -1 &&
			Item_getQuality(&search->current) != (uint8_t)search->quality)
			continue;
		if (search->frame != 255 && typeFrame.frame() != (uint16_t)(search->frame))
			continue;
		if ((search->flags & 1) && !FoundEquipped(search))
			continue;
		if ((search->flags & 2) && FoundEquipped(search))
			continue;
		if ((search->flags & 4) && !HAS_FLAG(typeFrame, CLASS_NPC))
			continue;
		if ((search->flags & 8) && (!HAS_FLAG(typeFrame, CLASS_NPC) || NPC_FLAG(&ref, NPC_DEAD)))
			continue;
		if (!(search->flags & 0x10) && HAS_FLAG(typeFrame, 0x400))
			continue;
		if (FLAG(Item_getQualityFlags(&search->current), QUALITY_INVISIBLE)) {
			if (HAS_FLAG(typeFrame, CLASS_NPC) && NPC_FLAG(&ref, NPC_IN_PARTY)) {
				if (!(search->flags & 0x60))
					continue;
			} else if (!(search->flags & 0x20))
				continue;
		}
		if (!(search->flags & 0x80) && (int8_t) gItemTypeInfo[typeFrame.type()].transparent)
			continue;
		return 1;
	}
}

int8_t FindNearestItem(AreaSearch *search, Loc x, Loc y, int16_t radius, int16_t flags, int16_t type, int8_t quality,
	int16_t frame)
{
	int16_t best = 0;
	int16_t distance;
	int16_t bestDistance = 0x7fff;

	for (FindItemInArea(search, WrapCoord(x.value - radius), WrapCoord(y.value - radius),
			WrapCoord(x.value + radius), WrapCoord(y.value + radius), flags, type, quality, frame);
		IS_VALID(search->current.off); FindItem(search)) {
		distance = Item_greatestDeltaToCoords(search->current, x, y, 0);
		if (distance < bestDistance) {
			bestDistance = distance;
			best = search->current.off;
		}
	}
	search->current.off = best;
	return search->current.off != 0;
}

int8_t FindItemAtPointInZRange(AreaSearch *search, int16_t *x, int16_t *y, int16_t flags, int16_t type, int8_t quality,
	int16_t frame, uint16_t minZ, uint16_t maxZ)
{
	uint16_t z;

	search->minZ = minZ;
	search->maxZ = maxZ;
	for (FindItemInArea(search, *x, *y, flags, type, quality, frame);
		search->found() && ((z = FoundZ(search)) < search->minZ || z > search->maxZ);
		FindItem(search))
		;
	return search->current.off != 0;
}

int8_t FindItemInArea(AreaSearch *search, Loc x0, Loc y0, Loc x1, Loc y1, int16_t flags, int16_t type, int8_t quality,
	int16_t frame, uint16_t minZ, uint16_t maxZ)
{
	uint16_t z;

	search->minZ = minZ;
	search->maxZ = maxZ;
	for (FindItemInArea(search, x0, y0, x1, y1, flags, type, quality, frame);
		IS_VALID(search->current.off) && ((z = FoundZ(search)) < search->minZ || z > search->maxZ);
		FindItem(search))
		;
	return search->current.off != 0;
}

int8_t FindNearestItemInZRange(AreaSearch *search, Loc x, Loc y, int16_t radius, int16_t flags, int16_t type,
	int8_t quality, int16_t frame, uint16_t minZ, uint16_t maxZ)
{
	int16_t best = 0;
	int16_t distance;
	int16_t bestDistance = 0x7fff;
	uint16_t z;

	search->minZ = minZ;
	search->maxZ = maxZ;
	for (FindItemInArea(search, WrapCoord(x.value - radius), WrapCoord(y.value - radius),
			WrapCoord(x.value + radius), WrapCoord(y.value + radius), flags, type, quality, frame);
		IS_VALID(search->current.off); FindItem(search)) {
		distance = Item_greatestDeltaToCoords(search->current, x, y, 0);
		if (distance < bestDistance && (z = FoundZ(search)) >= search->minZ && z <= search->maxZ) {
			bestDistance = distance;
			best = search->current.off;
		}
	}
	search->current.off = best;
	return search->current.off != 0;
}

objref FindItemInChunkLists(int16_t type, int8_t quality, int16_t frame)
{
	objref ref;
	TypeFrame typeFrame;
	int16_t i, x, y;

	for (i = 0; i < 4; i++)
		for (y = 0; y < 16; y++)
			for (x = 0; x < 16; x++) {
				ref = ChunkItemLists[i][y][x];
				/* ref never advances, so a miss here loops forever */
				while (((int8_t) (ref.off != 0))) {
					typeFrame = ITEM(ref.off)->typeFrame;
					if ((type == -1 || typeFrame.type() == (uint16_t)type) &&
						(quality == (int8_t) -1 || (HasQuality(&ref) && Item_getQuality(&ref) == (uint8_t)quality)) &&
						(frame == 255 || typeFrame.frame() == (uint16_t)frame))
						return ref;
				}
			}
	return ref;
}
