#ifndef SLIME_H
#define SLIME_H

struct Coord;
struct Loc;
struct ItemId;
struct objref;

void OnStrangeMoverRemoved(objref ref);
void OnStrangeMoverPlaced(objref ref, Coord *x, Coord *y);
unsigned char OnStrangeMoverStep(objref ref, Loc, Loc, int);
unsigned char HasEvenCellPlacement(int type);
unsigned char HasFixedFrames(int type);
int GetStrangeMoverFrame(ItemId id, int value);
ItemId GetStrangeMoverTarget(ItemId original);

#endif
