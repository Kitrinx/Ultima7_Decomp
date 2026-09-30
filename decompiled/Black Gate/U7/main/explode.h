#ifndef EXPLODE_H
#define EXPLODE_H

struct Loc;
struct objref;

extern unsigned char ExplosionRadii[];
extern int ExplosiveTypes[];
extern unsigned char ExplosionEffects[];
extern char ExplosionSounds[];

void EmptyExplodeStub();
void Explode(objref attacker, Loc x, Loc y, int width, int weaponNum, int ammoNum, objref projectile);

/* Defined with the powder keg. */
extern unsigned char PowderKegExploding;
void far ExplodePowderKeg(objref source);

#endif
