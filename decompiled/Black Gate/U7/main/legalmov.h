#ifndef LEGALMOV_H
#define LEGALMOV_H

#include "typefram.h"
#include "objref.h"

struct Loc;
struct CellCoord;
struct ItemId;

/* CheckMove's result: the z change in the low byte, and these */
#define MOVE_CLEAR      0x8000
#define MOVE_HAZARD     0x4000
#define MOVE_DOOR       0x2000

unsigned far CheckMove(CellCoord x, CellCoord y, int z, TypeFrame far &typeFrame,
	char direction, int distance, unsigned movement);
unsigned char far CanWalkAt(Loc x, Loc y, int z, TypeFrame far &typeFrame);
unsigned char far CanTypeMoveTo(CellCoord x, CellCoord y, int z, TypeFrame far &typeFrame);
unsigned char far CanFlyAt(Loc x, Loc y, int z, TypeFrame far &typeFrame);
unsigned char far CanSwimAt(Loc x, Loc y, int z, TypeFrame far &typeFrame);
unsigned char far CanItemMoveTo(CellCoord x, CellCoord y, int z, ItemId item);

/* The same test for an item. */
inline unsigned char CanTypeMoveTo(CellCoord x, CellCoord y, int z, ItemId item)
{
	return CanItemMoveTo(x, y, z, item);
}

#endif
