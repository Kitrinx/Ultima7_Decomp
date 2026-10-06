#ifndef COMBWPN_H
#define COMBWPN_H

struct Coord;
struct objref;

void far ApplyMonsterHit(objref attacker, objref target, char damage);
void far ApplyHitAtCoords(objref attacker, Coord x, Coord y, int z, int weapon, int ammo, int missile);
void far ApplyWeaponHit(objref attacker, objref target, int weapon, int ammo, int missile, unsigned char fromExplosion);
void far BreakHitItem(objref ref);

extern objref HitTargetItem;
extern Coord HitX;
extern Coord HitY;
extern int HitMissile;
extern unsigned char HitFromExplosion;
extern int NaturalDamage;
extern int HitZ;
extern int HitSourceItem;

#endif
