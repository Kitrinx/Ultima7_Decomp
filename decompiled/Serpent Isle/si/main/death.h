#ifndef DEATH_H
#define DEATH_H

struct Coord;
struct NPCRef;
struct objref;

void far AwardExperience(objref npc, long amount);
int far GetCombatStrength(NPCRef who, int type);
int far KnockOut(NPCRef who, char hp);
int far KillNpc(objref target, int attacker);
void far RefitInventory(objref item);
int far GetBodyNpc(objref body);
char far ResurrectBody(objref body, Coord x, Coord y, int z);
int far GetDeadPartyList(objref **list);
void far ClearDeadPartyList(void);

#endif
