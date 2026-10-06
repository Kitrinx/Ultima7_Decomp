#ifndef ITEM_H
#define ITEM_H

#include "itemrec.h"
#include "iteminfo.h"

struct Coord;
struct Loc;
struct CellCoord;

/* The 8-byte extension a container or NPC record points to. */
struct ItemExtra {
	int contents;
	unsigned char region, quality, flags, z, hitPoints, qualityFlags;
};

/* bits of an item's quality flags */
#define QUALITY_INVISIBLE       0x01
#define QUALITY_OKAY_TO_TAKE    0x08
#define QUALITY_BUSY            0x20
#define QUALITY_WEAPON_READY    0x40
#define QUALITY_CARRIES_LIGHT   0x80

extern ItemRecord far *ItemBuffer;
extern int ItemFreeList;
extern objref DetachedItems;
extern int ItemFreeCount;

#define ITEM(off) ((struct ItemRecord far *)MK_FP(ItemBufferSegment, off))
#define EXTRA(off) ((struct ItemExtra far *)MK_FP(ItemBufferSegment, off))

void far Item_setFrame(objref *ref, int frame);
Coord far Item_getX(objref &ref);
Coord far Item_getY(objref &ref);
void far Item_getXAndY(objref &ref, int *x, int *y);
void far SetItemZAndStuff(objref *ref, ItemInfo far &info);
void far Item_setTemporary(objref *ref);
void far Item_clearTemporary(objref *ref);
int far Item_hasHitPoints(objref *ref);
unsigned char far Item_hasQuantity(objref *ref);
unsigned char far Item_hasQuality(objref *ref);
unsigned char far Item_isOkayToTake(objref *ref);
void far Item_setOkayToTake(objref *ref);
void far Item_clearOkayToTake(objref *ref);
void far Item_setWeaponReady(objref *ref);
void far Item_clearWeaponReady(objref &ref);
void far Item_setCarriesLight(objref *ref);
void far Item_clearCarriesLight(objref *ref);
void far Item_setLocationKind(objref *ref, unsigned char value);
void far MaskItemZAndStuff(objref *ref, unsigned char value);
void far OrItemZAndStuff(objref *ref, unsigned char value);
void far Item_markEquipped(objref *ref);
void far Item_unequip(objref *ref);
char far Item_getHitPoints(objref *ref);
void far Item_setHitPoints(objref *ref, unsigned char value);
unsigned char far Item_getQualityFlags(objref *ref);
void far Item_setQualityFlags(objref *ref, unsigned char value);
unsigned char far Item_getQuantity(objref *ref);
void far Item_storeQuantity(objref *ref, unsigned char quantity);
unsigned char far Item_getQuality(objref *ref);

/* An NPC's combat mode is kept in its quality. */
inline unsigned char IsInMode(objref *who, unsigned char mode) { return Item_getQuality(who) == mode; }

unsigned char far Item_getCharges(objref *ref);
void far Item_setQuality(objref *ref, char quality);
unsigned char far Item_getRegion(objref *ref);
void far Item_setRegion(objref *ref, unsigned char region);
int far GetItemRecordCount(int classFlags);
void far AllocateItemRecords(objref *ref, int count);
int far FindRegionSlot(Loc x, Loc y);
void far Item_linkIntoContainer(objref *ref, objref container);
unsigned char far CreateItem(objref *ref, TypeFrame type, CellCoord x, CellCoord y, int z);
unsigned char far CreateItemInContainer(objref *ref, TypeFrame type, objref container);
int far *far Item_findLink(objref *ref);
char far Item_spillContents(objref *ref);
char far Item_deleteContents(objref *ref);
objref far Item_getContainer(objref *ref);
void far Item_setContainer(objref *ref, objref parent);
objref far GetContainedItem(objref *ref);
int far Item_getExtraRecord(objref *ref);
void far Item_setContents(objref *ref, objref contents);
unsigned char far Item_isWithin(objref *ref, objref container);
void far FreeItemRecord(objref *ref);
char far Item_delete(objref *ref);
void far Item_setZ(objref *ref, int z);
unsigned char far Item_move(objref *ref, CellCoord x, CellCoord y);
unsigned char far Item_move(objref *ref, Loc x, Loc y, int z);
unsigned char far Item_forceMove(objref *ref, Loc x, Loc y, int z);
void far Item_move(objref *ref, unsigned char dir);
void far Item_move(objref *ref, unsigned char dir, int dz);
void far Item_forceMove(objref *ref, unsigned char dir, int dz);
unsigned char far Item_moveIntoContainer(objref *ref, objref container);
unsigned char far Item_detach(objref *ref);
unsigned char far CreateItem(objref *ref, TypeFrame frame);
unsigned char far PlaceItemDirect(objref *ref, Loc x, Loc y, int z);
unsigned char far PlaceItem(objref *ref, Loc x, Loc y, int z);
unsigned char far PlaceItemInContainer(objref *ref, objref container);
unsigned char far PlaceItemOffMap(objref *ref);
unsigned char far ZapDetachedItem(objref *ref);
int far GetItemBeingDragged(objref *ref);
unsigned char far IsItemDetached(objref *ref);
int far CountDetachedItems();
extern "C" int far Item_greatestDeltaToItem(objref &ref, objref other);
void far Item_setInvisible(objref *ref);
int far Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, unsigned targetZ);
unsigned char far Item_getDirToItem(objref *item, objref other, unsigned char cardinal);
unsigned char far GetDiagonalStep(int major, int minor);
unsigned char far Item_getDirToCoords(objref *item, Coord x, Coord y, unsigned char cardinal);
void far CheckItemHandle(unsigned handle);
void far CheckItemLists(void);
void far ReportBadFreeList(char *file, int line);

extern unsigned ItemBufferSegment;
extern int OffMapItemLink;
extern int ChunkItemLists[4][16][16];
ItemInfo far GetItemZAndStuff(objref *ref);

#endif
