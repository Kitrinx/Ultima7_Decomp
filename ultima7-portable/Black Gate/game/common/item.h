#ifndef ITEM_H
#define ITEM_H

#include "itemrec.h"
#include "iteminfo.h"

struct Coord;
struct Loc;
struct CellCoord;

/* The 8-byte extension a container or NPC record points to. */
struct ItemExtra {
	int16_t contents;
	uint8_t region, quality, flags, z, hitPoints, qualityFlags;
};

/* bits of an item's quality flags */
#define QUALITY_INVISIBLE       0x01
#define QUALITY_OKAY_TO_TAKE    0x08
#define QUALITY_BUSY            0x20
#define QUALITY_WEAPON_READY    0x40
#define QUALITY_CARRIES_LIGHT   0x80

extern ItemRecord *ItemBuffer;
extern int16_t ItemFreeList;
extern objref DetachedItems;
extern int16_t ItemFreeCount;

#define ITEM(off) ((struct ItemRecord *)ItemAt(off))
#define EXTRA(off) ((struct ItemExtra *)ItemAt(off))

void Item_setFrame(objref *ref, int16_t frame);
Coord Item_getX(objref &ref);
Coord Item_getX(objref &&ref);
Coord Item_getY(objref &ref);
Coord Item_getY(objref &&ref);
void Item_getXAndY(objref &ref, int16_t *x, int16_t *y);
inline void Item_getXAndY(objref &&ref, int16_t *x, int16_t *y) { Item_getXAndY(ref, x, y); }
void SetItemZAndStuff(objref *ref, ItemInfo &info);
inline void SetItemZAndStuff(objref *ref, ItemInfo &&info) { SetItemZAndStuff(ref, info); }
void Item_setTemporary(objref *ref);
void Item_clearTemporary(objref *ref);
int16_t Item_hasHitPoints(objref *ref);
uint8_t Item_hasQuantity(objref *ref);
uint8_t Item_hasQuality(objref *ref);
uint8_t Item_isOkayToTake(objref *ref);
void Item_setOkayToTake(objref *ref);
void Item_clearOkayToTake(objref *ref);
void Item_setWeaponReady(objref *ref);
void Item_clearWeaponReady(objref &ref);
inline void Item_clearWeaponReady(objref &&ref) { Item_clearWeaponReady(ref); }
void Item_setCarriesLight(objref *ref);
void Item_clearCarriesLight(objref *ref);
void Item_setLocationKind(objref *ref, uint8_t value);
void MaskItemZAndStuff(objref *ref, uint8_t value);
void OrItemZAndStuff(objref *ref, uint8_t value);
void Item_markEquipped(objref *ref);
void Item_unequip(objref *ref);
int8_t Item_getHitPoints(objref *ref);
void Item_setHitPoints(objref *ref, uint8_t value);
uint8_t Item_getQualityFlags(objref *ref);
void Item_setQualityFlags(objref *ref, uint8_t value);
uint8_t Item_getQuantity(objref *ref);
void Item_storeQuantity(objref *ref, uint8_t quantity);
uint8_t Item_getQuality(objref *ref);

/* An NPC's combat mode is kept in its quality. */
inline uint8_t IsInMode(objref *who, uint8_t mode) { return Item_getQuality(who) == mode; }

uint8_t Item_getCharges(objref *ref);
void Item_setQuality(objref *ref, int8_t quality);
uint8_t Item_getRegion(objref *ref);
void Item_setRegion(objref *ref, uint8_t region);
int16_t GetItemRecordCount(int16_t classFlags);
void AllocateItemRecords(objref *ref, int16_t count);
int16_t FindRegionSlot(Loc x, Loc y);
void Item_linkIntoContainer(objref *ref, objref container);
uint8_t CreateItem(objref *ref, TypeFrame type, CellCoord x, CellCoord y, int16_t z);
uint8_t CreateItemInContainer(objref *ref, TypeFrame type, objref container);
int16_t * Item_findLink(objref *ref);
int8_t Item_spillContents(objref *ref);
int8_t Item_deleteContents(objref *ref);
objref Item_getContainer(objref *ref);
void Item_setContainer(objref *ref, objref parent);
objref GetContainedItem(objref *ref);
int16_t Item_getExtraRecord(objref *ref);
void Item_setContents(objref *ref, objref contents);
uint8_t Item_isWithin(objref *ref, objref container);
void FreeItemRecord(objref *ref);
int8_t Item_delete(objref *ref);
void Item_setZ(objref *ref, int16_t z);
uint8_t Item_move(objref *ref, CellCoord x, CellCoord y);
uint8_t Item_move(objref *ref, Loc x, Loc y, int16_t z);
uint8_t Item_forceMove(objref *ref, Loc x, Loc y, int16_t z);
void Item_move(objref *ref, uint8_t dir);
void Item_move(objref *ref, uint8_t dir, int16_t dz);
void Item_forceMove(objref *ref, uint8_t dir, int16_t dz);
uint8_t Item_moveIntoContainer(objref *ref, objref container);
uint8_t Item_detach(objref *ref);
uint8_t CreateItem(objref *ref, TypeFrame frame);
uint8_t PlaceItemDirect(objref *ref, Loc x, Loc y, int16_t z);
uint8_t PlaceItem(objref *ref, Loc x, Loc y, int16_t z);
uint8_t PlaceItemInContainer(objref *ref, objref container);
uint8_t PlaceItemOffMap(objref *ref);
uint8_t ZapDetachedItem(objref *ref);
int16_t GetItemBeingDragged(objref *ref);
uint8_t IsItemDetached(objref *ref);
int16_t CountDetachedItems();
extern "C" int16_t Item_greatestDeltaToItem(objref &ref, objref other);
void Item_setInvisible(objref *ref);
int16_t Item_greatestDeltaToCoords(objref &ref, CellCoord targetX, CellCoord targetY, uint16_t targetZ);
uint8_t Item_getDirToItem(objref *item, objref other, uint8_t cardinal);
uint8_t GetDiagonalStep(int16_t major, int16_t minor);
uint8_t Item_getDirToCoords(objref *item, Coord x, Coord y, uint8_t cardinal);
void CheckItemHandle(uint16_t handle);
void CheckItemLists(void);
void ReportBadFreeList(char *file, int16_t line);

extern uint8_t *ItemBufferBase;
extern int16_t OffMapItemLink;
extern int16_t ChunkItemLists[4][16][16];
extern int16_t UnusedItemGlobal;
ItemInfo GetItemZAndStuff(objref *ref);

#endif
