#ifndef ATTACK_H
#define ATTACK_H

struct objref;

unsigned char far IsAvatarDead();

extern unsigned char AvatarStepRequested, AutorouteActive;
void far UpdateNPCs(unsigned char avatarStepping);

extern unsigned char SchedulePeriod;
void far StopFlanking(objref *npc);
void far KeepNearAvatar(objref *npc);
void far IdleInCombat(objref *npc);
void far UpdateCombatNPCs();

#endif
