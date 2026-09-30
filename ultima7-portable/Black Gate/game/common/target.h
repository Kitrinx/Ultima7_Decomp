#ifndef TARGET_H
#define TARGET_H

struct NPCPose;

class ShapeManager;
struct WorldView;
struct objref;

int8_t FindItemAtScreenPoint(objref *hit, int16_t x, int16_t y, WorldView *view, ShapeManager *shapes, int8_t opaqueOnly);

extern int32_t ShadowNpcBuffer;

void AllocateNPCPoses(int32_t *table);
void GetNPCPose(int32_t *table, int16_t i, struct NPCPose *buf);

struct Coord;

int8_t HavePlayerSelect(objref *picked, Coord *px, Coord *py, int16_t *pz);

#endif
