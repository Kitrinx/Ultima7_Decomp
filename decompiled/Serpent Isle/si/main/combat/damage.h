#ifndef DAMAGE_H
#define DAMAGE_H

#include "objref.h"

struct NPCRef;

int far ReduceHealth(objref target, int damage, int type, objref attacker);
int far CountWeaponAmmo(objref npc, int weapon);
unsigned far GetWeaponRange(objref npc, int weapon);
int far DealDamage(unsigned char strength, int damage, int type, objref target, objref attacker);

unsigned char far RollToWin(unsigned char attack, unsigned char defence);

#ifdef __cplusplus
extern "C" {
#endif
unsigned char far RollToHit(struct NPCRef attacker, struct objref target, int bonus);
#ifdef __cplusplus
}
#endif

int far FindWeaponAmmo(objref npc, int weaponNumber, objref far &ammo);
void far ShowHarmlessHit(objref target);

#endif
