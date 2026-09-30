#ifndef DAZE_H
#define DAZE_H

/* Spell and status effects on items and NPCs. */
struct objref;
struct NPCRef;

int far SplitCreature(objref);
unsigned char far PutToSleep(NPCRef);
void far ApplyCurse(NPCRef);
unsigned char far ApplyPoison(NPCRef);
void far ApplyParalysis(NPCRef);
void far EndCurse(NPCRef);
void far EndPoison(NPCRef);
void far EndParalysis(NPCRef);

void far ApplyCharm(objref *ref, unsigned char mode);
void far EndSleep(objref ref);
void far EndCharm(objref ref);

unsigned char far FallToGround(objref ref);

#endif
