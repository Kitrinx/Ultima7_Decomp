#ifndef COLLIDE_H
#define COLLIDE_H

/* Collision tests and placement on the map grid. */
struct CellCoord;
struct TypeFrame;
struct objref;
struct ItemId;
struct Coord;

extern uint8_t BlockedByDoor, InDungeon;
extern int32_t HeightMasks[16];

void AllocateCollisionBuffer();
int8_t WorldToCollisionCell(CellCoord x, CellCoord y, int16_t *dx, int16_t *dy, int16_t margin);
uint8_t IsTypeBlockedAt(CellCoord x, CellCoord y, int16_t z, TypeFrame typeFrame);
uint8_t IsBoxBlockedAt(CellCoord x, CellCoord y, int16_t z, int16_t w, int16_t h, int16_t height);
uint8_t IsTypeSupportedAt(CellCoord, CellCoord, int16_t, TypeFrame);
int16_t FindSupportLevel(CellCoord, CellCoord, int16_t, TypeFrame *);
void AddTypeToCollision(CellCoord, CellCoord, int16_t, TypeFrame);
void AddTypeToCollision(objref);
void RemoveTypeFromCollision(CellCoord, CellCoord, int16_t, TypeFrame);
void RemoveTypeFromCollision(objref);
void ClearCollisionBuffer();
void AddChunkToCollision(CellCoord, CellCoord);
void UpdateCeiling(ItemId);
uint8_t IsPointInItem(ItemId id, Coord x, Coord y, int16_t z);
void SetInDungeon(uint8_t inside);
uint8_t IsUnderMountain(ItemId);
void UpdateDungeonState(ItemId id);
void SettleChunkItems(CellCoord x, CellCoord y);
void ApplyContactEffect(objref npc, objref item);
void CheckContactEffects(objref npc);

#endif
