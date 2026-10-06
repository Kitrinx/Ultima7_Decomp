#ifndef NPCSHAPE_H
#define NPCSHAPE_H

#include "objref.h"

void far RemapObjectShape(int shape, int source);
void far RestorePolymorphs(void);
void far SetNpcAppearance(objref npc, unsigned shape);
void far WorldToRegionCoordinates(int *x, int *y, int *map);
void far CheckMoonshadeRavaged(void);

#endif
