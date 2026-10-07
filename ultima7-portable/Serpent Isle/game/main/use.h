#ifndef USE_H
#define USE_H

struct Coord;
struct objref;

uint8_t Item_canBeOpened(objref object);
void Use(objref *object, Coord x, Coord y, int16_t z);
void Item_setDefaultAttackMode(objref object, uint8_t mode);
uint16_t Item_getDefaultAttackMode(objref object);
void Item_setAttackMode(objref object, uint8_t mode);
uint16_t Item_getAttackMode(objref object);
uint8_t Item_isPartyMember(objref object);

#endif
