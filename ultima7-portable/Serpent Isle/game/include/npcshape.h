#ifndef NPCSHAPE_H
#define NPCSHAPE_H

#include "objref.h"

void RemapObjectShape(int16_t shape, int16_t source);
void RestorePolymorphs(void);
void SetNpcAppearance(objref npc, uint16_t shape);
void WorldToRegionCoordinates(int16_t *x, int16_t *y, int16_t *map);
void CheckMoonshadeRavaged(void);

#endif
