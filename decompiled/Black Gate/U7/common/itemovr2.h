#ifndef ITEMOVR2_H
#define ITEMOVR2_H

struct objref;

void far LoadNpcs(char *filename);
void far ResetNpcRecords(objref *manager);
void far AllocNpcRecords(objref *manager, int count, int extra);
void far RelinkNpcsInArea();
void far RebuildNpcFreeLists(objref *actor);
void far EquipCarriedItems(objref *actor);
void far SaveNpcs(char *filename);
void far ResetObjectLists();

struct WorldView;

void far InitItemManager(WorldView *view);

#endif
