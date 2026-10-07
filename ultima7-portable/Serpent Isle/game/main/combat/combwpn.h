#ifndef COMBWPN_H
#define COMBWPN_H

struct Coord;
struct objref;

void ApplyMonsterHit(objref attacker, objref target, int8_t damage);
void ApplyHitAtCoords(objref attacker, Coord x, Coord y, int16_t z, int16_t weapon, int16_t ammo, int16_t missile);
void ApplyWeaponHit(objref attacker, objref target, int16_t weapon, int16_t ammo, int16_t missile, uint8_t fromExplosion);
void BreakHitItem(objref ref);

extern objref HitTargetItem;
extern Coord HitX;
extern Coord HitY;
extern int16_t HitMissile;
extern uint8_t HitFromExplosion;
extern int16_t NaturalDamage;
extern int16_t HitZ;
extern int16_t HitSourceItem;

#endif
