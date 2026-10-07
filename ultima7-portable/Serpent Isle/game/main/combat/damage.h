#ifndef DAMAGE_H
#define DAMAGE_H

#include "objref.h"

struct NPCRef;

int16_t ReduceHealth(objref target, int16_t damage, int16_t type, objref attacker);
int16_t CountWeaponAmmo(objref npc, int16_t weapon);
uint16_t GetWeaponRange(objref npc, int16_t weapon);
int16_t DealDamage(uint8_t strength, int16_t damage, int16_t type, objref target, objref attacker);

uint8_t RollToWin(uint8_t attack, uint8_t defence);

#ifdef __cplusplus
extern "C" {
#endif
uint8_t RollToHit(struct NPCRef attacker, struct objref target, int16_t bonus);
#ifdef __cplusplus
}
#endif

int16_t FindWeaponAmmo(objref npc, int16_t weaponNumber, objref &ammo);
void ShowHarmlessHit(objref target);

#endif
