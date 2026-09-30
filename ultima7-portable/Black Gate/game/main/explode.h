#ifndef EXPLODE_H
#define EXPLODE_H

struct Loc;
struct objref;

extern uint8_t ExplosionRadii[];
extern int16_t ExplosiveTypes[];
extern uint8_t ExplosionEffects[];
extern char ExplosionSounds[];

void EmptyExplodeStub();
void Explode(objref attacker, Loc x, Loc y, int16_t width, int16_t weaponNum, int16_t ammoNum, objref projectile);

/* Defined with the powder keg. */
extern uint8_t PowderKegExploding;
void ExplodePowderKeg(objref source);

#endif
