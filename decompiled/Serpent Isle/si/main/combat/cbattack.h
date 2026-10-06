#ifndef CBATTACK_H
#define CBATTACK_H

struct Coord;
struct objref;

char far AttackItemWithWeapon(objref actor, objref target, int weapon);
char far AttackCoordsWithWeapon(objref actor, Coord x, Coord y, int z, int weapon);
char far StrikeItem(objref actor, objref target);
char far StrikeCoords(objref actor, Coord x, Coord y, int z);
unsigned char far StrikeItemWithWeapon(objref actor, objref target, int weapon);
char far StrikeCoordsWithWeapon(objref actor, Coord x, Coord y, int z, int weapon);

extern int StrikeTargetX;
extern int StrikeTargetY;
extern int StrikeTargetZ;
extern int AttackTargetZ;
extern objref AttackTargetItem;
extern Coord AttackTargetX;
extern Coord AttackTargetY;
extern objref StrikeTargetItem;
char far AttackItem(objref actor, objref target);
char far AttackCoords(objref actor, Coord x, Coord y, int z);

#endif
