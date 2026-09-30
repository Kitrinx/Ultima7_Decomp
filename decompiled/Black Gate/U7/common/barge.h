#ifndef BARGE_H
#define BARGE_H

struct far TypeFrame;

struct objref;
struct Coord;

/* A barge is a ship, cart or carpet whose parts move as one; its extra record holds size and facing. */
unsigned char far Barge_getWidth(objref &barge);
void far Barge_setWidth(objref &barge, char width);
unsigned char far Barge_getHeight(objref &barge);
void far Barge_setHeight(objref &barge, char height);
unsigned char far Barge_getDir(objref &barge);
void far Barge_setDir(objref &barge, char facing);
char far Barge_canMove(objref &barge, unsigned char dir, int dist);
char far Barge_move(objref &barge, unsigned char dir, int dist);
char far Barge_canTurn(objref &barge, unsigned char dir);
char far Barge_turn(objref &barge, unsigned char dir);
unsigned char far Barge_isStripFree(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2);
unsigned char far Barge_checkTerrain(objref &barge, Coord x1, Coord y1, Coord x2, Coord y2, int mode);
char far Barge_rise(objref &barge);
char far Barge_descend(objref &barge);
char far Barge_isOkayToLand(objref &barge);

void far LeaveVehicle();

objref far FindBargeUnder(objref item);

extern unsigned char BargeAnimationDue;
TypeFrame far RotateShapeQuarter(TypeFrame typeFrame);
TypeFrame far RotateShapeHalf(TypeFrame typeFrame);
TypeFrame far RotateShapeBack(TypeFrame typeFrame);

#endif
