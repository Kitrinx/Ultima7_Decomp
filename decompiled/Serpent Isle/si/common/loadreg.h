#ifndef LOADREG_H
#define LOADREG_H

extern unsigned char CurrentRegion;
extern unsigned char SavedRegionBits[32];
extern char *IregPathFormat;
extern char *MapPath;

void far ForgetSavedRegions(void);
void far EmptyRegionStub(void);
void far SaveRegion(unsigned char *region, int slot, char unload);
void far LoadRegionMap(unsigned char *region, void far *buf);
void far LoadRegion(unsigned char *region, int slot);
void far EmptyRegionHook(unsigned char *region, void far *buf);

extern unsigned char ReloadingTerrain;
extern unsigned char unused_global_4;
void ReloadRegionTerrain(void);

#endif
