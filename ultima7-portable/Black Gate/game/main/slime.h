#ifndef SLIME_H
#define SLIME_H

struct Coord;
struct Loc;
struct ItemId;
struct objref;

void OnStrangeMoverRemoved(objref ref);
void OnStrangeMoverPlaced(objref ref, Coord *x, Coord *y);
uint8_t OnStrangeMoverStep(objref ref, Loc, Loc, int16_t);
uint8_t HasEvenCellPlacement(int16_t type);
uint8_t HasFixedFrames(int16_t type);
int16_t GetStrangeMoverFrame(ItemId id, int16_t value);
ItemId GetStrangeMoverTarget(ItemId original);

#endif
