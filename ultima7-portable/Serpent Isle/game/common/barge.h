#ifndef BARGE_H
#define BARGE_H

struct TypeFrame;

struct objref;
struct Coord;

/* A barge is a ship, cart or carpet whose parts move as one; its extra record holds size and facing. */
uint8_t Barge_getWidth(objref &barge);
void Barge_setWidth(objref &barge, int8_t width);
uint8_t Barge_getHeight(objref &barge);
void Barge_setHeight(objref &barge, int8_t height);
uint8_t Barge_getDir(objref &barge);
void Barge_setDir(objref &barge, int8_t facing);
int8_t Barge_canMove(objref &barge, uint8_t dir, int16_t dist);
int8_t Barge_move(objref &barge, uint8_t dir, int16_t dist);
int8_t Barge_canTurn(objref &barge, uint8_t dir);
int8_t Barge_turn(objref &barge, uint8_t dir);
uint8_t Barge_isStripFree(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2);
uint8_t Barge_checkTerrain(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2, int16_t mode);
int8_t Barge_rise(objref &barge);
int8_t Barge_descend(objref &barge);
int8_t Barge_isOkayToLand(objref &barge);
inline int8_t Barge_isOkayToLand(objref &&barge) { return Barge_isOkayToLand(barge); }

void LeaveVehicle();

objref FindBargeUnder(objref item);

extern uint8_t BargeAnimationDue;
TypeFrame RotateShapeQuarter(TypeFrame typeFrame);
TypeFrame RotateShapeHalf(TypeFrame typeFrame);
TypeFrame RotateShapeBack(TypeFrame typeFrame);

#endif
