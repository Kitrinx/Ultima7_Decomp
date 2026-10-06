#ifndef CRIMEOV1_H
#define CRIMEOV1_H

#include "crime.h"

void far GatherRegionalGuards(char *count, char required, unsigned char *quiet);
unsigned char far IsRegionalGuard(int number, unsigned char *quiet);
void far SpawnRegionalGuards(objref *origin, unsigned action);
void far DismissRegionalGuards();
void far SetGuardsOnAvatar();
void far ReportCrime(int thing, char hostile, char runUsable);

#endif
