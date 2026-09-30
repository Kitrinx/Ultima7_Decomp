#ifndef WEIGHT_H
#define WEIGHT_H

struct WeightVolume;

struct objref;

int far GetItemTypeWeight(int type);
int far Item_getWeight(objref item);
int far GetItemTypeBulk(int type);
void far LoadWgtVol(char far *name);
int far DetermineWeightOfContents(objref item);
int far DetermineBulkOfContents(objref item);

extern WeightVolume far *TypeWeightVolumes;

#endif
