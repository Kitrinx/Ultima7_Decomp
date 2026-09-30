#ifndef CRIME_H
#define CRIME_H

struct Combat;
struct CombatSaver;
struct objref;

int far GetDominantHostileSide();
void far SetGuardsOnAvatar();
extern char AvatarInCombat;
extern unsigned char KillNpcMode;
void far ReportCrime(int thing, char hostile, char runUsable);
int far GetGroupLeader(unsigned char alignment);
void far SetGroupLeader(int number);
void far ClearGroupLeader(unsigned char alignment);
void far ReleaseProtectors(unsigned char alignment, int target);
void far CharmNpc(objref *npc, unsigned char side);
void far UncharmNpc(objref *npc);

extern unsigned char CrimeUsecodeOff;
extern Combat CombatGroups;
extern char *AvFlagsFileName;
extern CombatSaver AvFlagsFile;

#endif
