#ifndef TARGET_H
#define TARGET_H

struct NPCPose;

class ShapeManager;
struct WorldView;
struct objref;

char far FindItemAtScreenPoint(objref *hit, int x, int y, WorldView *view, ShapeManager *shapes, char opaqueOnly);

extern long ShadowNpcBuffer;

void far AllocateNPCPoses(long *table);
void far GetNPCPose(long *table, int i, struct NPCPose *buf);

struct Coord;

char far HavePlayerSelect(objref *picked, Coord *px, Coord *py, int *pz);

#endif
