#ifndef USE_H
#define USE_H

struct Coord;
struct objref;

unsigned char far Item_canBeOpened(objref object);
void far Use(objref *object, Coord x, Coord y, int z);
void far Item_setDefaultAttackMode(objref object, unsigned char mode);
unsigned far Item_getDefaultAttackMode(objref object);
void far Item_setAttackMode(objref object, unsigned char mode);
unsigned far Item_getAttackMode(objref object);
unsigned char far Item_isPartyMember(objref object);

#endif
