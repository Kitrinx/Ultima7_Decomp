#ifndef POLYMORP_H
#define POLYMORP_H

#include "objref.h"

void ChangeShape(int16_t shape, int16_t look);
void RestorePolymorphs(void);
void SetPolymorph(objref npc, uint16_t shape);
void GetRegionPosition(int16_t *x, int16_t *y, int16_t *map);
void CheckMoonshadeRavaged(void);

#endif
