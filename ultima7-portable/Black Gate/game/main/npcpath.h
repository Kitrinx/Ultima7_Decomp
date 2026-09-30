#ifndef NPCPATH_H
#define NPCPATH_H

struct PathSaver;
struct Coord;
struct CellCoord;
struct objref;

/* Walks npc toward x, y, z; the route length goes to *length. 0 started, 1 no free slot, 2 no route. */
int8_t StartPath(objref npc, CellCoord x, CellCoord y, int8_t z, int16_t limit, int16_t *length, int8_t hold);
uint8_t CanFindPath(objref npc, Coord x, Coord y, int8_t z, int16_t *distance, int16_t limit);
void EndPathSlot(int16_t slot);
uint8_t HasPath(objref npc);
void StopPaths(objref npc);

extern int16_t NearestPathDistance[2];
extern PathSaver SavedPaths;

#endif
