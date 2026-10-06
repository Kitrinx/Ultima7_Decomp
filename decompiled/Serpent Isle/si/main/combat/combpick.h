#ifndef COMBPICK_H
#define COMBPICK_H

struct objref;

int far FindLeaderAttacker(objref *self);
int far FindNearestNpc(objref *self, int x, int y, unsigned char alignments, int modes, char targeted);

struct Coord;
struct NPCRef;

void far RallyProtectors(NPCRef &self);
unsigned char far FindGroupCentre(objref *self, Coord *x, Coord *y, unsigned char hostile);
int far ChooseRandomTarget(objref *self, char allowTargeted, char allowHelpless, int distance);
int far ChooseNearestTarget(objref *self, char allowTargeted, char allowHelpless, int distance);
int far ChooseWeakestTarget(objref *self, char allowTargeted, char allowHelpless, int distance);
int far ChooseStrongestTarget(objref *self, char allowTargeted, char allowHelpless, int distance);
unsigned char far ChooseTarget(NPCRef &self, int distance);
int far FindIdleProtector(objref *self);
void far ReassignProtector(objref *self);
void far ProtectLeader(NPCRef *self);
void far ChooseNearbyTarget(objref *self);

unsigned char far AreEnemies(objref *self, objref other);

#endif
