#ifndef LEGALMOV_H
#define LEGALMOV_H

#include "typefram.h"
#include "objref.h"
#include "coord.h"

struct Loc;
struct CellCoord;
struct ItemId;

/* CheckMove's result: the z change in the low byte, and these */
#define MOVE_CLEAR      0x8000
#define MOVE_HAZARD     0x4000
#define MOVE_DOOR       0x2000

uint16_t CheckMove(CellCoord x, CellCoord y, int16_t z, TypeFrame &typeFrame,
	int8_t direction, int16_t distance, uint16_t movement);
uint8_t CanWalkAt(Loc x, Loc y, int16_t z, TypeFrame &typeFrame);
uint8_t CanTypeMoveTo(CellCoord x, CellCoord y, int16_t z, TypeFrame &typeFrame);
uint8_t CanFlyAt(Loc x, Loc y, int16_t z, TypeFrame &typeFrame);
uint8_t CanSwimAt(Loc x, Loc y, int16_t z, TypeFrame &typeFrame);
uint8_t CanItemMoveTo(CellCoord x, CellCoord y, int16_t z, ItemId item);

inline uint16_t CheckMove(CellCoord x, CellCoord y, int16_t z, TypeFrame &&typeFrame,
	int8_t direction, int16_t distance, uint16_t movement)
{
	return CheckMove(x, y, z, typeFrame, direction, distance, movement);
}
inline uint8_t CanWalkAt(Loc x, Loc y, int16_t z, TypeFrame &&typeFrame) { return CanWalkAt(x, y, z, typeFrame); }
inline uint8_t CanTypeMoveTo(CellCoord x, CellCoord y, int16_t z, TypeFrame &&typeFrame)
{
	return CanTypeMoveTo(x, y, z, typeFrame);
}
inline uint8_t CanFlyAt(Loc x, Loc y, int16_t z, TypeFrame &&typeFrame) { return CanFlyAt(x, y, z, typeFrame); }
inline uint8_t CanSwimAt(Loc x, Loc y, int16_t z, TypeFrame &&typeFrame) { return CanSwimAt(x, y, z, typeFrame); }

/* The same test for an item. */
inline uint8_t CanTypeMoveTo(CellCoord x, CellCoord y, int16_t z, ItemId item)
{
	return CanItemMoveTo(x, y, z, item);
}

#endif
