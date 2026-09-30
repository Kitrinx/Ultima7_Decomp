/* Black Gate U7.EXE, resident segment 96 (file offsets 0x0335f5 to 0x035db6, 10177 bytes).
 * Borland C++ 2.0 -mm -O -G -P -Z rebuilds it byte for byte as C++.
 */

#include "itemrec.h"
#include "iteminfo.h"
#include "type.h"
#include "typefram.h"
#include "npcref.h"
#include "u7npc.h"
#include "slime.h"
#include "u7ibuf.h"
#include "random.h"
#include "cullmask.h"
#include "daze.h"
#include "trigger.h"
#include "u7sound.h"
#include "coord.h"
#include "maps.h"
#include "mapview.h"
#include "target.h"
#include "sortitem.h"
#include "search.h"
#include "wihh.h"

#define ITEM(r) ((struct ItemRecord far *) MK_FP(ItemBufferSegment, (r).off))
#define ITEM_TYPE(r) (ITEM(r)->typeFrame & 0x3ff)
#define FRAME(r) ((ITEM(r)->typeFrame & 0x7c00) >> 10)
#define IS_NPC(r) ((unsigned char) ((ItemTypeClassFlags[gItemTypeInfo[ITEM_TYPE(r)].typeClass] & CLASS_NPC) != 0))
#define FLAG(f, mask) ((unsigned char) ((f) & (mask)))
#define IS_CLASS(r, c) ((char) (gItemTypeInfo[ITEM_TYPE(r)].typeClass == (c)))
#define IS_SET(f, mask) ((char) (((f) & (mask)) != 0))

#define FEET_SLOT 11

/* how far each chunk of the view has been sorted */
#define SORT_WAITING    0
#define SORT_READY      1
#define SORT_ACTIVE     2
#define SORT_HELD       3   /* held by an item reaching into one neighbouring chunk */
#define SORT_HELD_BOTH  4   /* or into both */
#define SORT_DONE       5

/* terrain that can poison an NPC walking on it, and the boots that stop it */
#define SWAMP_TYPE      22
#define MUCK_TYPE       67
#define BUBBLES_TYPE    334     /* and the type after it */
#define SWAMP_BOOTS_TYPE 588

#define MIRROR_TYPE     268
#define OTHER_MIRROR_TYPE 848

/* an item's footprint: cell masks across and down, and the heights it covers */
struct ShapeExtent {
	unsigned char x, y;
	long xMask, yMask;
	unsigned zMask;
	unsigned char z;
};

/* how an NPC was last drawn */
struct ViewRecord {
	unsigned typeFrame;
	Coord x;
	Coord y;
	unsigned char z;
	ViewRecord() {}
};

/* rebuilds an object in storage it already has */
inline void *operator new(unsigned, void *p) { return p; }

struct NpcView : ViewRecord {
	NpcView() : ViewRecord() {}
	NpcView(objref r);
};

extern objref DetachedItems;
extern int ChunkItemLists[4][16][16];

extern int far FindRegionSlot(Loc, Loc);
extern Coord far Item_getX(objref &);
extern Coord far Item_getY(objref &);
extern void far Item_setFrame(objref *, int);
extern void far Item_setLocationKind(objref *, unsigned char);
extern void far Item_setRegion(objref *, unsigned char);
extern void far AddTypeToCollision(objref);
extern unsigned char far IsBoxBlockedAt(CellCoord x, CellCoord y, int z, int w, int h, int height);
extern unsigned char far IsTypeBlockedAt(CellCoord x, CellCoord y, int z, TypeFrame typeFrame);
/* the same test sized from a type */
inline unsigned char IsBoxBlockedAt(Loc x, Loc y, int z, TypeFrame far &s) { return IsTypeBlockedAt(x, y, z, s); }
extern void far ApplyContactEffect(objref, objref);

/* one step in each of the eight directions, north first, then none */
int DirDeltaX[9] = { 0, 1, 1, 1, 0, -1, -1, -1, 0 };
int DirDeltaY[9] = { -1, -1, 0, 1, 1, 1, 0, -1, 0 };
/* the same for the four straight directions */
int CardinalDeltaX[5] = { 0, 1, 0, -1, 0 };
int CardinalDeltaY[5] = { -1, 0, 1, 0, 0 };
objref RenderItems[1024];
RenderOrder ItemRenderOrder(RenderItems, 1024);
unsigned char ContactFound = 0;
objref ContactItem, ContactOther;
Coord RenderOriginX, RenderOriginY;
/* a mask of n + 1 cells */
unsigned CellSpanMasks[8] = { 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };
/* the height bits covered by a type of each height */
unsigned ExtentHeightMasks[8] = { 0x8000, 0x4000, 0x6000, 0x7000, 0x7800, 0x7c00, 0x7e00, 0x7f00 };

/* Per chunk of the 5 by 5 view: the next item to place, its sort state and two held-back items. */
int ChunkNextItem[5][5];
unsigned char ChunkSortState[5][5];
int HeldItemFirst[5][5];
int HeldItemSecond[5][5];
int LastChunkColumn, LastChunkRow;
unsigned RenderBoxHeight, RenderBoxWidth, RenderMinZ, RenderMaxZ;

/* the height bits an item of this type covers above z */
inline unsigned HeightMask(unsigned typeFrame, int z)
{
	return ExtentHeightMasks[gItemTypeInfo[TypeFrame(typeFrame).type()].height] >> z;
}

inline EggRecord far *EGG(objref r)
{
	return (EggRecord far *) MK_FP(ItemBufferSegment, ITEM(r)->data.extra);
}

/* when an NPC and another object meet, trips the object (an egg the NPC sets off, or a harmful one) */
void far TripContactItem(void)
{
	objref npc = 0, other;
	char hit;

	if (IS_NPC(ContactItem)) {
		npc = ContactItem;
		other = ContactOther;
	} else if (IS_NPC(ContactOther)) {
		npc = ContactOther;
		other = ContactItem;
	}
	if (npc.valid() && !GetNpcBufferForIbo(&npc)->hasStatus(0x400) && !GetNpcBufferForIbo(&npc)->hasStatus(NPC_DEAD)) {
		hit = 0;
		if (IS_CLASS(other, TYPE_CLASS_EGG) && !EGG(other)->hatched) {
			if ((unsigned char) EGG(other)->criteria == CRITERIA_AVATAR_FOOTPAD)
				hit = npc.off == AvatarRef.off;
			else if ((unsigned char) EGG(other)->criteria == CRITERIA_PARTY_FOOTPAD)
				hit = GetNpcBufferForIbo(&npc)->hasStatus(NPC_IN_PARTY);
		}
		if (hit)
			ActivateEgg(other.off);
		else if ((unsigned char) gItemTypeInfo[ITEM_TYPE(other)].field)
			ApplyContactEffect(npc, other);
	}
}

void far SetItemExtent(ShapeExtent *box, objref r, unsigned char x, unsigned char y)
{
	unsigned typeFrame = ITEM(r)->typeFrame;

	box->x = x;
	box->y = y;
	box->z = (int)Item_getZ(&r);
	box->xMask = (long) CellSpanMasks[GetFootprintX(typeFrame)] << (box->x - ITEM(r)->position.world.x);
	box->yMask = (long) CellSpanMasks[GetFootprintY(typeFrame)] << (box->y - ITEM(r)->position.world.y);
	box->zMask = HeightMask(typeFrame, box->z);
	if (!ContactFound)
		ContactItem = r;
}

/* 1 if the object lies behind the box, 2 if in front of it, 0 if neither */
int far CompareItemExtent(ShapeExtent *box, objref r)
{
	unsigned typeFrame = ITEM(r)->typeFrame;
	long across, down;
	unsigned char z;
	unsigned mask;

	z = (int)Item_getZ(&r);
	across = (long) CellSpanMasks[GetFootprintX(typeFrame)] << (box->x - ITEM(r)->position.world.x);
	down = (long) CellSpanMasks[GetFootprintY(typeFrame)] << (box->y - ITEM(r)->position.world.y);
	mask = HeightMask(typeFrame, z);
	if (box->xMask <= across && box->yMask <= down && box->zMask <= mask) {
		if (!ContactFound && (across & box->xMask) && (down & box->yMask) && (box->zMask & mask)) {
			ContactFound = 1;
			ContactOther = r;
		}
		return 1;
	}
	if (box->xMask >= across && box->yMask >= down && box->zMask >= mask) {
		if (!ContactFound && (across & box->xMask) && (down & box->yMask) && (box->zMask & mask)) {
			ContactFound = 1;
			ContactOther = r;
		}
		return 2;
	}
	if (across & box->xMask) {
		if (down & box->yMask) {
			if (!ContactFound && (box->zMask & mask)) {
				ContactFound = 1;
				ContactOther = r;
			}
			if (box->zMask < mask)
				return 1;
			if (box->zMask > mask)
				return 2;
		} else {
			if (box->yMask < down && (box->zMask <= mask || (box->zMask & mask)))
				return 1;
			if (box->yMask > down && (box->zMask >= mask || (box->zMask & mask)))
				return 2;
		}
	} else {
		if (down & box->yMask) {
			if (box->xMask < across && (box->zMask <= mask || (box->zMask & mask)))
				return 1;
			if (box->xMask > across && (box->zMask >= mask || (box->zMask & mask)))
				return 2;
		} else if (box->xMask < across) {
			if (box->yMask < down && (box->zMask <= mask || (box->zMask & mask)))
				return 1;
		} else if (box->yMask > down && (box->zMask >= mask || (box->zMask & mask)))
			return 2;
	}
	return 0;
}

/* places an item taken from the free list at (x, y) in its chunk, in drawing order */
unsigned char far PlaceItem(objref *ref, CellCoord x, CellCoord y)
{
	objref far *head;
	int tx, ty, xEnd, yEnd, area;
	objref current, prev, before, found;
	ShapeExtent first, second;

	ContactFound = 0;
	*ref = DetachedItems;
	if (ref->valid()) {
		if ((unsigned char)gItemTypeInfo[ITEM_TYPE(*ref)].strangeMovement)
			OnStrangeMoverPlaced(*ref, &x, &y);
		area = FindRegionSlot(x, y);
		if (area != 255) {
			tx = (x & 0xff) >> 4;
			ty = (y & 0xff) >> 4;
			xEnd = (tx << 4) + 15;
			yEnd = (ty << 4) + 15;
			head = (objref *)&ChunkItemLists[area][ty][tx];
			DetachedItems = ITEM(*ref)->next;
			ITEM(*ref)->position.world.x = x;
			ITEM(*ref)->position.world.y = y;
			Item_setLocationKind(ref, area);
			if (FLAG(ItemTypeClassFlags[gItemTypeInfo[ITEM_TYPE(*ref)].typeClass], CLASS_REGION))
				Item_setRegion(ref, LoadedRegions[area]);
			prev = 0;
			ITEM(prev)->next = head->off;
			SetItemExtent(&first, *ref, xEnd, yEnd);
			for (current.off = head->off; current.valid() && CompareItemExtent(&first, current) != 1;
				prev = current, current = current.next())
				;
			ITEM(*ref)->next = current.off;
			ITEM(prev)->next = ref->off;
			if (current.valid()) {
				before = prev;
				prev = current;
				current = current.next();
				found = 0;
				while (current.valid()) {
					if (found.valid()) {
						if (current == found) {
							ITEM(prev)->next = ITEM(current)->next;
							ITEM(current)->next = ref->off;
							ITEM(before)->next = current.off;
							before = current;
							current = prev;
							found = 0;
						} else if (CompareItemExtent(&second, current) == 2) {
							found = current;
							SetItemExtent(&second, current, xEnd, yEnd);
							prev = *ref;
							current = prev.next();
						}
					} else if (CompareItemExtent(&first, current) == 2) {
						found = current;
						SetItemExtent(&second, current, xEnd, yEnd);
						prev = *ref;
						current = prev.next();
					}
					prev = current;
					current = current.next();
				}
			}
			prev = 0;
			head->off = ITEM(prev)->next;
			ITEM(prev)->next = 0;
			AddTypeToCollision(*ref);
			if (IS_NPC(*ref)) {
				objref npc = *ref;
				GetNpcBufferForIbo(&npc)->changeFlags(0x400, 0);
				if (Item_getZ(ref) == 0) {
					objref worn;
					TypeFrame typeFrame;
					int startX, mapY, width, height, column, row;
					char hazard;
					int mapX;
					npc = *ref;
					mapX = startX = GetDelta(x, CellWindowX);
					mapY = GetDelta(y, CellWindowY);
					typeFrame = ITEM(*ref)->typeFrame;
					width = GetFootprintX(typeFrame) + 1;
					height = GetFootprintY(typeFrame) + 1;
					hazard = 0;
					for (row = 0; row < height; row++) {
						mapX = startX;
						for (column = 0; column < width; column++) {
							if ((unsigned char)gItemTypeInfo[CellBuffer[mapY][mapX] & 0x3ff].field) {
								hazard = 1;
								break;
							}
							if (--mapX < 0)
								break;
						}
						if (hazard || --mapY < 0)
							break;
					}
					if (hazard) {
						switch (CellBuffer[mapY][mapX] & 0x3ff) {
						case SWAMP_TYPE:
						case MUCK_TYPE:
						case BUBBLES_TYPE:
						case BUBBLES_TYPE + 1:
							if (GenerateRandomIntegerInRange(100) < 3) {
								worn = GetItemInSlot(npc, FEET_SLOT);
								if (!(worn.valid() && ITEM_TYPE(worn) == SWAMP_BOOTS_TYPE
									|| FLAG(GetNpcBufferForIbo(&npc)->typeFlags, NPC_FLY))) {
									ApplyPoison((NPCRef &)npc);
									if (IS_SET(GetNpcBufferForIbo(&npc)->status, NPC_POISONED))
										PlayPainSfx();
								}
							}
						}
					}
				}
			}
			if (ContactFound)
				TripContactItem();
			return 1;
		}
		if (IS_NPC(*ref)) {
			DetachedItems = ITEM(*ref)->next;
			ITEM(*ref)->position.world.x = x;
			ITEM(*ref)->position.world.y = y;
			ITEM(*ref)->next = 0;
			Item_setLocationKind(ref, 4);    /* off the map */
			Item_setRegion(ref, GetRegionAt(x, y, CurrentMap));
			if ((unsigned char)gItemTypeInfo[ITEM_TYPE(*ref)].strangeMovement)
				OnStrangeMoverPlaced(*ref, &x, &y);
			return 1;
		}
	}
	return 0;
}

unsigned char AreaScan::start(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int topZ, int bottomZ)
{
	x = x0;
	y = y0;
	width = GetDelta(x1, x0) + 1;
	height = GetDelta(y1, y0) + 1;
	left = x0 & 0xfff0;
	top = y0 & 0xfff0;
	right = x1 & 0xfff0;
	bottom = y1 & 0xfff0;
	this->topZ = topZ;
	this->bottomZ = bottomZ;
	cy = bottom;
	cx = Coord(right + 16);
	current = 0;
	started = 1;
	return nextChunk();
}

char AreaScan::first(Coord x0, Coord y0, Coord x1, Coord y1, int topZ, int bottomZ)
{
	if (!start(x0, y0, x1, y1, topZ, bottomZ))
		return 0;
	return next();
}

unsigned char AreaScan::nextChunk()
{
	int chunk;
	int col;
	int row;

	while (!current.valid()) {
		if (cx == left) {
			cx = right;
			if (cy == top) {
				current = 0;
				break;
			}
			cy -= 16;
		} else
			cx -= 16;
		chunk = FindRegionSlot(cx, cy);
		if (chunk != 255) {
			col = (cx & 0xff) >> 4;
			row = (cy & 0xff) >> 4;
			current = ChunkItemLists[chunk][row][col];
		}
	}
	return current.valid();
}

char AreaScan::next()
{
	int z;

	while (current.valid()) {
		if (!started)
			current = ITEM(current)->next;
		started = 0;
		if (nextChunk()) {
			if (GetDelta(Item_getX(current), x) >= width)
				continue;
			if (GetDelta(Item_getY(current), y) >= height)
				continue;
			if ((z = (int)Item_getZ(&current)) >= bottomZ && z <= topZ)
				break;
		}
	}
	return current.valid();
}

void far UpdateMirrorFrame(objref r)
{
	if (FRAME(r) < 2) {
		Coord x, y;

		x = Item_getX(r);
		y = Item_getY(r);
		if (ITEM_TYPE(r) == MIRROR_TYPE) {
			if (IsBoxBlockedAt(x + 2, y, 1, 2, 2, 2))
				Item_setFrame(&r, 1);
			else
				Item_setFrame(&r, 0);
		} else {
			if (IsBoxBlockedAt(x, y + 2, 1, 2, 2, 2))
				Item_setFrame(&r, 1);
			else
				Item_setFrame(&r, 0);
		}
	}
}

inline NpcView::NpcView(objref r) : ViewRecord()
{
	typeFrame = ITEM(r)->typeFrame;
	x = Item_getX(r);
	y = Item_getY(r);
	z = (int)Item_getZ(&r);
}

/* adds an object inside the view box to the list; 0 when the list is full */
unsigned char RenderOrder::addItem(objref r)
{
	int z;
	NpcView view;
	unsigned type;

	if (r.valid()) {
		if (AvatarDontMove && (unsigned char)Item_isAvatar(&NPCRef(r)))
			return 1;
		if (GetDelta(Item_getX(r), RenderOriginX) >= RenderBoxWidth)
			return 1;
		if (GetDelta(Item_getY(r), RenderOriginY) >= RenderBoxHeight)
			return 1;
		z = (int)Item_getZ(&r);
		if (z >= RenderMinZ && z <= RenderMaxZ) {
			if ((char *) back - (char *) front >= sizeof(objref)) {
				*front++ = r.off;
				AddOccluder(r.off);
				type = ITEM_TYPE(r);
				if (type == MIRROR_TYPE || type == OTHER_MIRROR_TYPE) {
					UpdateMirrorFrame(r);
				}
				if (IS_NPC(r)) {
					new (&view) NpcView(r);
					StoreNpcViewRecord(&ShadowNpcBuffer, Item_getNpcNumber(&NPCRef(r)), &view);
				}
			} else
				return 0;
		}
	}
	return 1;
}

objref RenderOrder::first()
{
	objref r;

	if (begin != front) {
		cur = begin;
		r = *cur;
	} else
		r = 0;
	return r;
}

objref RenderOrder::last()
{
	objref r;

	if (begin != front) {
		cur = front - 1;
		r = *cur;
	} else
		r = 0;
	return r;
}

objref RenderOrder::previous()
{
	objref r;

	if (cur > begin) {
		cur--;
		r = *cur;
	} else
		r = 0;
	return r;
}

objref RenderOrder::nextItem()
{
	objref r;

	if (cur + 1 < front) {
		cur++;
		r = *cur;
	} else
		r = 0;
	return r;
}

char far Item_crossesChunkX(objref r)
{
	unsigned typeFrame = ITEM(r)->typeFrame;
	unsigned char x = ITEM(r)->position.world.x;

	return ShapeCrossesChunkX(typeFrame, x);
}

unsigned char far ShapeCrossesChunkX(TypeFrame far &typeFrame, unsigned char x)
{
	if (((x - GetFootprintX(typeFrame)) & 0xfff0) != (x & 0xfff0))
		return 1;
	return 0;
}

char far Item_crossesChunkY(objref r)
{
	unsigned typeFrame = ITEM(r)->typeFrame;
	unsigned char y = ITEM(r)->position.world.y;

	return ShapeCrossesChunkY(typeFrame, y);
}

unsigned char far ShapeCrossesChunkY(TypeFrame far &typeFrame, unsigned char y)
{
	if (((y - GetFootprintY(typeFrame)) & 0xfff0) != (y & 0xfff0))
		return 1;
	return 0;
}

/* whether b hides behind a, measured from the far corner of their chunks */
unsigned char far IsItemHiddenBehind(objref a, objref b)
{
	int x = ITEM(a)->position.world.x & 0xfff0;
	int y = ITEM(a)->position.world.y & 0xfff0;
	int otherX = ITEM(b)->position.world.x & 0xfff0;
	int otherY = ITEM(b)->position.world.y & 0xfff0;
	ShapeExtent box;

	if ((char) (otherX - x) > 0)
		x = otherX;
	if ((char) (otherY - y) > 0)
		y = otherY;
	SetItemExtent(&box, a, x + 15, y + 15);
	return CompareItemExtent(&box, b) == 1;
}

/* takes a held-back object out of the next chunks down, right and diagonally once it is placed */
unsigned char far ReleaseHeldItem(int col, int row, objref r)
{
	unsigned char placed = 0;

	if (row < LastChunkRow && r == ChunkNextItem[row + 1][col]) {
		if (ChunkSortState[row + 1][col] == SORT_HELD) {
			placed = 1;
			ChunkNextItem[row + 1][col] = ITEM(r)->next;
			ChunkSortState[row + 1][col] = SORT_READY;
		}
	} else if (col < LastChunkColumn && r == ChunkNextItem[row][col + 1]) {
		if (ChunkSortState[row][col + 1] == SORT_HELD) {
			placed = 1;
			ChunkNextItem[row][col + 1] = ITEM(r)->next;
			ChunkSortState[row][col + 1] = SORT_READY;
		}
	} else if (row < LastChunkRow && col < LastChunkColumn && r == ChunkNextItem[row + 1][col + 1]
		&& ChunkSortState[row + 1][col + 1] == SORT_HELD_BOTH) {
		placed = 1;
		ChunkNextItem[row + 1][col + 1] = ITEM(r)->next;
		ChunkSortState[row + 1][col + 1] = SORT_READY;
		if (r == HeldItemFirst[row + 1][col]) {
			HeldItemFirst[row + 1][col] = HeldItemSecond[row + 1][col];
			HeldItemSecond[row + 1][col] = 0;
		}
		if (r == HeldItemSecond[row + 1][col])
			HeldItemSecond[row + 1][col] = 0;
		if (r == HeldItemFirst[row][col + 1]) {
			HeldItemFirst[row][col + 1] = HeldItemSecond[row][col + 1];
			HeldItemSecond[row][col + 1] = 0;
		}
		if (r == HeldItemSecond[row][col + 1])
			HeldItemSecond[row][col + 1] = 0;
	}
	return placed;
}

/* fills the list with the objects in the view box, each after the ones it hides */
unsigned char RenderOrder::build(CellCoord x0, CellCoord y0, CellCoord x1, CellCoord y1, int maxZ, int minZ)
{
	int firstHit, secondHit;
	unsigned char done, flag;
	int mode;
	Coord left, top, right, bottom, cx, cy;
	int chunk, ix, iy;
	objref cur, other;
	int swap;
	ShapeExtent firstBox, secondBox;
	int col, row;

	ContactFound = 0;
	RenderMaxZ = maxZ;
	RenderMinZ = minZ;
	RenderOriginX = x0;
	RenderOriginY = y0;
	front = begin;
	back = end;
	ClearOcclusionMask();
	SetOcclusionOrigin(x0, y0);
	RenderBoxWidth = GetDelta(x1, x0) + 1;
	RenderBoxHeight = GetDelta(y1, y0) + 1;
	left = x0 & 0xfff0;
	top = y0 & 0xfff0;
	right = x1 & 0xfff0;
	bottom = y1 & 0xfff0;

	cy = top;
	for (row = 0; row < 5; row++) {
		cx = left;
		for (col = 0; col < 5; col++) {
			chunk = FindRegionSlot(cx, cy);
			if (chunk != 255) {
				ix = (cx & 0xff) >> 4;
				iy = (cy & 0xff) >> 4;
				ChunkNextItem[row][col] = ChunkItemLists[chunk][iy][ix];
			} else
				ChunkNextItem[row][col] = 0;
			HeldItemFirst[row][col] = 0;
			HeldItemSecond[row][col] = 0;
			ChunkSortState[row][col] = SORT_WAITING;
			if (cx == right) {
				LastChunkColumn = col;
				break;
			}
			cx += 16;
		}
		if (cy == bottom) {
			LastChunkRow = row;
			break;
		}
		cy += 16;
	}

	ChunkSortState[LastChunkRow][LastChunkColumn] = SORT_READY;
	cur = 0;
	col = LastChunkColumn;
	row = LastChunkRow;
	cur = 0;
	done = 0;
	mode = 0;
	while (!done) {
		if (!cur.valid()) {
			if (mode != 0) {
				if (mode == 2) {
					cur = HeldItemFirst[row][col];
					other = HeldItemSecond[row][col];
					if (IsItemHiddenBehind(other, cur)) {
						cur = other;
						other = HeldItemFirst[row][col];
					}
					HeldItemFirst[row][col] = 0;
					HeldItemSecond[row][col] = 0;
					if (ReleaseHeldItem(col, row, cur) && !addItem(cur))
						return 0;
					if (ReleaseHeldItem(col, row, other) && !addItem(other))
						return 0;
				} else {
					cur = HeldItemFirst[row][col];
					HeldItemFirst[row][col] = 0;
					if (ReleaseHeldItem(col, row, cur) && !addItem(cur))
						return 0;
				}
				mode = 0;
				if (HeldItemFirst[row][col] == 0 && HeldItemSecond[row][col] != 0) {
					HeldItemFirst[row][col] = HeldItemSecond[row][col];
					HeldItemSecond[row][col] = 0;
				}
				cur = 0;
				continue;
			}
			col = LastChunkColumn;
			row = LastChunkRow;
			while (ChunkSortState[row][col] != SORT_READY)
				if (--col < 0) {
					col = LastChunkColumn;
					if (--row < 0) {
						done = 1;
						break;
					}
				}
			if (done) {
				done = 0;
				col = LastChunkColumn;
				row = LastChunkRow;
				while (ChunkSortState[row][col] == SORT_DONE)
					if (--col < 0) {
						col = LastChunkColumn;
						if (--row < 0) {
							done = 1;
							break;
						}
					}
				if (ChunkSortState[row][col] == SORT_HELD || ChunkSortState[row][col] == SORT_HELD_BOTH) {
					ChunkSortState[row][col] = SORT_READY;
					cur = ChunkNextItem[row][col];
					if (!addItem(cur))
						return 0;
					cur = cur.next();
					ChunkNextItem[row][col] = cur.off;
					if (col > 0) {
						HeldItemFirst[row][col - 1] = 0;
						HeldItemSecond[row][col - 1] = 0;
					}
					if (row > 0) {
						HeldItemFirst[row - 1][col] = 0;
						HeldItemSecond[row - 1][col] = 0;
					}
				}
			}
			if (done)
				break;
			cur = HeldItemFirst[row][col];
			if (cur.valid()) {
				mode = 1;
				SetItemExtent(&firstBox, cur, (ITEM(cur)->position.world.x & 0xf0) + 15,
					(ITEM(cur)->position.world.y & 0xf0) + 15);
				cur = HeldItemSecond[row][col];
				if (cur.valid()) {
					mode = 2;
					SetItemExtent(&secondBox, cur, (ITEM(cur)->position.world.x & 0xf0) + 15,
						(ITEM(cur)->position.world.y & 0xf0) + 15);
				}
			} else
				mode = 0;
			cur = ChunkNextItem[row][col];
			ChunkSortState[row][col] = SORT_ACTIVE;
			if (row > 0 && (col == LastChunkColumn || ChunkSortState[row - 1][col + 1] > SORT_READY)
				&& ChunkSortState[row - 1][col] < SORT_HELD)
				ChunkSortState[row - 1][col] = SORT_READY;
			if (col > 0 && (row == LastChunkRow || ChunkSortState[row + 1][col - 1] > SORT_READY)
				&& ChunkSortState[row][col - 1] < SORT_HELD)
				ChunkSortState[row][col - 1] = SORT_READY;
		}
		if (!cur.valid())
			ChunkSortState[row][col] = SORT_DONE;
		else if (gItemTypeInfo[ITEM_TYPE(cur)].height == 0) {
			cur = cur.next();
			ChunkNextItem[row][col] = cur.off;
			if (!cur.valid())
				ChunkSortState[row][col] = SORT_DONE;
		} else {
			flag = 0;
			firstHit = secondHit = 0;
			if (mode != 0) {
				firstHit = CompareItemExtent(&firstBox, cur);
			}
			if (mode == 2) {
				secondHit = CompareItemExtent(&secondBox, cur);
			}
			if (firstHit == 1 || secondHit == 1) {
				ChunkNextItem[row][col] = cur.off;
				ChunkSortState[row][col] = SORT_READY;
				flag = 1;
			} else if (firstHit != 2 && secondHit != 2) {
				if (col > 0 && Item_crossesChunkX(cur)) {
					if (mode != 0) {
						firstHit = 1;
						if (mode == 2)
							secondHit = 1;
						flag = 1;
					}
					ChunkNextItem[row][col] = cur.off;
					if (HeldItemFirst[row][col - 1] == 0)
						HeldItemFirst[row][col - 1] = cur.off;
					else if (HeldItemSecond[row][col - 1] == 0)
						HeldItemSecond[row][col - 1] = cur.off;
					if (row > 0 && Item_crossesChunkY(cur)) {
						ChunkSortState[row][col] = SORT_HELD_BOTH;
						if (HeldItemFirst[row - 1][col] == 0)
							HeldItemFirst[row - 1][col] = cur.off;
						else if (HeldItemSecond[row - 1][col] == 0)
							HeldItemSecond[row - 1][col] = cur.off;
						if (HeldItemFirst[row - 1][col - 1] == 0)
							HeldItemFirst[row - 1][col - 1] = cur.off;
						else if (HeldItemSecond[row - 1][col - 1] == 0)
							HeldItemSecond[row - 1][col - 1] = cur.off;
					} else {
						ChunkSortState[row][col] = SORT_HELD;
						if (row > 0 && ChunkSortState[row - 1][col] == SORT_READY)
							ChunkSortState[row - 1][col] = SORT_WAITING;
					}
				} else if (row > 0 && Item_crossesChunkY(cur)) {
					if (mode != 0) {
						firstHit = 1;
						if (mode == 2)
							secondHit = 1;
						flag = 1;
					}
					ChunkNextItem[row][col] = cur.off;
					ChunkSortState[row][col] = SORT_HELD;
					if (col > 0 && ChunkSortState[row][col - 1] == SORT_READY)
						ChunkSortState[row][col - 1] = SORT_WAITING;
					if (HeldItemFirst[row - 1][col] == 0)
						HeldItemFirst[row - 1][col] = cur.off;
					else if (HeldItemSecond[row - 1][col] == 0)
						HeldItemSecond[row - 1][col] = cur.off;
				}
				if (!flag && (ChunkSortState[row][col] == SORT_HELD || ChunkSortState[row][col] == SORT_HELD_BOTH)) {
					cur = 0;
					mode = 0;
					continue;
				}
			}
			if (flag) {
				if (firstHit == 1) {
					if (secondHit == 1 && IsItemHiddenBehind(HeldItemSecond[row][col], HeldItemFirst[row][col])) {
						swap = HeldItemFirst[row][col];
						HeldItemFirst[row][col] = HeldItemSecond[row][col];
						HeldItemSecond[row][col] = swap;
					}
					cur = HeldItemFirst[row][col];
					HeldItemFirst[row][col] = 0;
					if (ReleaseHeldItem(col, row, cur) && !addItem(cur))
						return 0;
				}
				if (secondHit == 1) {
					cur = HeldItemSecond[row][col];
					HeldItemSecond[row][col] = 0;
					if (ReleaseHeldItem(col, row, cur) && !addItem(cur))
						return 0;
				}
				if (HeldItemFirst[row][col] == 0 && HeldItemSecond[row][col] != 0) {
					HeldItemFirst[row][col] = HeldItemSecond[row][col];
					HeldItemSecond[row][col] = 0;
				}
				mode = 0;
				cur = 0;
			} else {
				if (!addItem(cur))
					return 0;
				cur = cur.next();
				if (!cur.valid()) {
					ChunkNextItem[row][col] = 0;
					ChunkSortState[row][col] = SORT_DONE;
				}
			}
		}
	}

	cy = top;
	for (row = 0; row < 5; row++) {
		cx = left;
		for (col = 0; col < 5; col++) {
			chunk = FindRegionSlot(cx, cy);
			if (chunk != 255) {
				ix = (cx & 0xff) >> 4;
				iy = (cy & 0xff) >> 4;
				for (cur = ChunkItemLists[chunk][iy][ix]; cur.valid(); cur = cur.next())
					if (gItemTypeInfo[ITEM_TYPE(cur)].height == 0 && !addItem(cur))
						return 0;
			}
			if (cx == right) {
				LastChunkColumn = col;
				break;
			}
			cx += 16;
		}
		if (cy == bottom) {
			LastChunkRow = row;
			break;
		}
		cy += 16;
	}
	if (ContactFound)
		TripContactItem();
	return 1;
}
