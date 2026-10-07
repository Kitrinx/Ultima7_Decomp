#ifndef ITEMOVR2_H
#define ITEMOVR2_H

struct objref;

void LoadNpcs(char *filename);
void ResetNpcRecords(objref *manager);
void AllocNpcRecords(objref *manager, int16_t count, int16_t extra);
void RelinkNpcsInArea();
void RebuildNpcFreeLists(objref *actor);
void EquipCarriedItems(objref *actor);
void SaveNpcs(char *filename);
void ResetObjectLists();

struct WorldView;

void InitItemManager(WorldView *view);

#endif
