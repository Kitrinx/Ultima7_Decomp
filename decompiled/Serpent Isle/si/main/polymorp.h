#ifndef POLYMORP_H
#define POLYMORP_H

#include "objref.h"

void ChangeShape(int shape, int look);
void RestorePolymorphs(void);
void SetPolymorph(objref npc, unsigned shape);
void GetRegionPosition(int *x, int *y, int *map);
void CheckMoonshadeRavaged(void);

#endif
