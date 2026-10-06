#ifndef COMBATAI_H
#define COMBATAI_H

/* NPC combat and schedule helpers. */
struct objref;
struct NPCRef;
struct Coord;

extern "C" void far HandleNpcHit(objref, objref, char);
int far GetNpcWeapon(objref *);
unsigned far GetAttackRange(NPCRef *);
int far GetCombatTarget(objref *);
void far RemoveFromCombat(NPCRef &);
void far Npc_setTarget(NPCRef *, int, char);
unsigned char far PathNextToItem(objref *, NPCRef, int);
unsigned char far FindSpotNextToItem(objref *, objref, Coord *, Coord *, int *);
unsigned char far IsSentient(NPCRef &npc);
unsigned char far CanMove(objref *);
unsigned char far PrepareAttack(NPCRef *);
unsigned char far StepToward(objref *, Coord, Coord, int, int);
unsigned char far ApproachTarget(NPCRef *);
void far TakeCombatTurn(NPCRef *);
unsigned char far FleeStep(objref *, char, char, char);
int far GetMoveStride(objref *);

extern char ProtectChance[];
extern unsigned char PartyMissileFlags[];
extern unsigned char FleeDirections[8][5];
extern char CallForHelpChance[];
unsigned far GetRangeWithWeapon(NPCRef &npc, int *weapon);
void far CallForHelp(NPCRef &self);
unsigned char far IsTargetInReach(NPCRef *attacker, objref target);
char far IsRangedAttack(objref *npc, int weaponNumber, objref target);
extern int InvisibleTypes[];
char far CanTurnInvisible(objref *p);
extern int TeleportingTypes[];
char far CanTeleport(objref *p);
extern int SummoningTypes[];
char far CanSummon(objref *p);
unsigned char far RespondToAttack(NPCRef &self, int attacker, char hp);
unsigned char far GetDirectionTo(objref *from, objref to);

int far Npc_getScheduleVariable0(objref *);
int far Npc_getScheduleVariable1(objref *);
unsigned char far Npc_getMovementRadius(objref *);

#endif
