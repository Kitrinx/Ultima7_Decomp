#ifndef COLLIDE_H
#define COLLIDE_H

/* Collision tests and placement on the map grid. */
struct CellCoord;
struct far TypeFrame;
struct objref;
struct ItemId;
struct Coord;

extern unsigned char BlockedByDoor, InDungeon;
extern long HeightMasks[16];

void far AllocateCollisionBuffer();
char far WorldToCollisionCell(CellCoord x, CellCoord y, int *dx, int *dy, int margin);
unsigned char far IsTypeBlockedAt(CellCoord x, CellCoord y, int z, TypeFrame typeFrame);
unsigned char far IsBoxBlockedAt(CellCoord x, CellCoord y, int z, int w, int h, int height);
unsigned char far IsTypeSupportedAt(CellCoord, CellCoord, int, TypeFrame);
int far FindSupportLevel(CellCoord, CellCoord, int, TypeFrame far *);
void far AddTypeToCollision(CellCoord, CellCoord, int, TypeFrame);
void far AddTypeToCollision(objref);
void far RemoveTypeFromCollision(CellCoord, CellCoord, int, TypeFrame);
void far RemoveTypeFromCollision(objref);
void far ClearCollisionBuffer();
void far AddChunkToCollision(CellCoord, CellCoord);
void far UpdateCeiling(ItemId);
unsigned char far IsPointInItem(ItemId id, Coord x, Coord y, int z);
void far SetInDungeon(unsigned char inside);
unsigned char far IsUnderMountain(ItemId);
void far UpdateDungeonState(ItemId id);
void far SettleChunkItems(CellCoord x, CellCoord y);
void far ApplyContactEffect(objref npc, objref item);
void far CheckContactEffects(objref npc);

#endif
