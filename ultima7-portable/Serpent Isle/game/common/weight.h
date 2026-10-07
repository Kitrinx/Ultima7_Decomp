#ifndef WEIGHT_H
#define WEIGHT_H

struct WeightVolume;

struct objref;

int16_t GetItemTypeWeight(int16_t type);
int16_t Item_getWeight(objref item);
int16_t GetItemTypeBulk(int16_t type);
void LoadWgtVol(char *name);
int16_t DetermineWeightOfContents(objref item);
int16_t DetermineBulkOfContents(objref item);

extern WeightVolume *TypeWeightVolumes;

#endif
