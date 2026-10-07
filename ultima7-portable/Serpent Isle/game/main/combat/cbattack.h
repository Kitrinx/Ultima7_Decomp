#ifndef CBATTACK_H
#define CBATTACK_H

struct Coord;
struct objref;

int8_t AttackItemWithWeapon(objref actor, objref target, int16_t weapon);
int8_t AttackCoordsWithWeapon(objref actor, Coord x, Coord y, int16_t z, int16_t weapon);
int8_t StrikeItem(objref actor, objref target);
int8_t StrikeCoords(objref actor, Coord x, Coord y, int16_t z);
uint8_t StrikeItemWithWeapon(objref actor, objref target, int16_t weapon);
int8_t StrikeCoordsWithWeapon(objref actor, Coord x, Coord y, int16_t z, int16_t weapon);

extern int16_t StrikeTargetX;
extern int16_t StrikeTargetY;
extern int16_t StrikeTargetZ;
extern int16_t AttackTargetZ;
extern objref AttackTargetItem;
extern Coord AttackTargetX;
extern Coord AttackTargetY;
extern objref StrikeTargetItem;
int8_t AttackItem(objref actor, objref target);
int8_t AttackCoords(objref actor, Coord x, Coord y, int16_t z);

#endif
