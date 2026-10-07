#ifndef CRIMEOV1_H
#define CRIMEOV1_H

#include "crime.h"

void GatherRegionalGuards(char *count, int8_t required, uint8_t *quiet);
uint8_t IsRegionalGuard(int16_t number, uint8_t *quiet);
void SpawnRegionalGuards(objref *origin, uint16_t action);
void DismissRegionalGuards();
void SetGuardsOnAvatar();
void ReportCrime(int16_t thing, int8_t hostile, int8_t runUsable);

#endif
