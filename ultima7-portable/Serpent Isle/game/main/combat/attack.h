#ifndef ATTACK_H
#define ATTACK_H

struct objref;

uint8_t IsAvatarDead();

extern uint8_t AvatarStepRequested, AutorouteActive;
void UpdateNPCs(uint8_t avatarStepping);

extern uint8_t SchedulePeriod;
void StopFlanking(objref *npc);
void KeepNearAvatar(objref *npc);
void IdleInCombat(objref *npc);
void UpdateCombatNPCs();

#endif
