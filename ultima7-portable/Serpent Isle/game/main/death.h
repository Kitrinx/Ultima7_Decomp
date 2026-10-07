#ifndef DEATH_H
#define DEATH_H

struct Coord;
struct NPCRef;
struct objref;

void AwardExperience(objref npc, int32_t amount);
int16_t GetCombatStrength(NPCRef who, int16_t type);
int16_t KnockOut(NPCRef who, int8_t hp);
int16_t KillNpc(objref target, int16_t attacker);
void RefitInventory(objref item);
int16_t GetBodyNpc(objref body);
int8_t ResurrectBody(objref body, Coord x, Coord y, int16_t z);
int16_t GetDeadPartyList(objref **list);
void ClearDeadPartyList(void);

#endif
