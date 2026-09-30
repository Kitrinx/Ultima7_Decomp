#ifndef COMBATAI_H
#define COMBATAI_H

/* NPC combat and schedule helpers. */
#include "npcref.h"

struct Coord;

extern "C" void HandleNpcHit(objref, objref, int8_t);
int16_t GetNpcWeapon(objref *);
uint16_t GetAttackRange(NPCRef *);
int16_t GetCombatTarget(objref *);
void RemoveFromCombat(NPCRef &);
inline void RemoveFromCombat(NPCRef &&npc) { RemoveFromCombat(npc); }
void Npc_setTarget(NPCRef *, int16_t, int8_t);
uint8_t PathNextToItem(objref *, NPCRef, int16_t);
uint8_t FindSpotNextToItem(objref *, objref, Coord *, Coord *, int16_t *);
uint8_t IsSentient(NPCRef &npc);
inline uint8_t IsSentient(NPCRef &&npc) { return IsSentient(npc); }
uint8_t CanMove(objref *);
uint8_t PrepareAttack(NPCRef *);
uint8_t StepToward(objref *, Coord, Coord, int16_t, int16_t);
uint8_t ApproachTarget(NPCRef *);
void TakeCombatTurn(NPCRef *);
uint8_t FleeStep(objref *, int8_t, int8_t, int8_t);
int16_t GetMoveStride(objref *);

extern char ProtectChance[];
extern uint8_t PartyMissileFlags[];
extern uint8_t FleeDirections[8][5];
extern char CallForHelpChance[];
uint16_t GetRangeWithWeapon(NPCRef &npc, int16_t *weapon);
void CallForHelp(NPCRef &self);
inline void CallForHelp(NPCRef &&self) { CallForHelp(self); }
uint8_t IsTargetInReach(NPCRef *attacker, objref target);
int8_t IsRangedAttack(objref *npc, int16_t weaponNumber, objref target);
extern int16_t InvisibleTypes[];
int8_t CanTurnInvisible(objref *p);
extern int16_t TeleportingTypes[];
int8_t CanTeleport(objref *p);
extern int16_t SummoningTypes[];
int8_t CanSummon(objref *p);
uint8_t RespondToAttack(NPCRef &self, int16_t attacker, int8_t hp);
inline uint8_t RespondToAttack(NPCRef &&self, int16_t attacker, int8_t hp) { return RespondToAttack(self, attacker, hp); }
uint8_t GetDirectionTo(objref *from, objref to);

#endif
