#ifndef CRIME_H
#define CRIME_H

struct Combat;
struct CombatSaver;
struct objref;

int16_t GetDominantHostileSide();
extern int8_t AvatarInCombat;
extern uint8_t KillNpcMode;
int16_t GetGroupLeader(uint8_t alignment);
void SetGroupLeader(int16_t number);
void ClearGroupLeader(uint8_t alignment);
void ReleaseProtectors(uint8_t alignment, int16_t target);
void CharmNpc(objref *npc, uint8_t side);
void UncharmNpc(objref *npc);

extern uint8_t CrimeUsecodeOff;
extern Combat CombatGroups;
extern char *const AvFlagsFileName;
extern CombatSaver AvFlagsFile;

#endif
