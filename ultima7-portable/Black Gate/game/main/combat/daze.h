#ifndef DAZE_H
#define DAZE_H

/* Spell and status effects on items and NPCs. */
struct objref;
struct NPCRef;

int16_t SplitCreature(objref);
uint8_t PutToSleep(NPCRef);
void ApplyCurse(NPCRef);
uint8_t ApplyPoison(NPCRef);
void ApplyParalysis(NPCRef);
void EndCurse(NPCRef);
void EndPoison(NPCRef);
void EndParalysis(NPCRef);

void ApplyCharm(objref *ref, uint8_t mode);
void EndSleep(objref ref);
void EndCharm(objref ref);

uint8_t FallToGround(objref ref);

#endif
