#ifndef NPCPATH_H
#define NPCPATH_H

struct PathSaver;
struct Coord;
struct CellCoord;
struct objref;

/* Walks npc toward x, y, z; the route length goes to *length. 0 started, 1 no free slot, 2 no route. */
char far StartPath(objref npc, CellCoord x, CellCoord y, char z, int limit, int *length, char hold);
unsigned char far CanFindPath(objref npc, Coord x, Coord y, char z, int *distance, int limit);
void far EndPathSlot(int slot);
unsigned char far HasPath(objref npc);
void far StopPaths(objref npc);

extern int NearestPathDistance[2];
extern PathSaver SavedPaths;

#endif
