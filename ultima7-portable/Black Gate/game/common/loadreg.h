#ifndef LOADREG_H
#define LOADREG_H

extern uint8_t CurrentRegion;
extern uint8_t SavedRegionBits[32];
extern char *IregPathFormat;
extern char *MapPath;

void ForgetSavedRegions(void);
void EmptyRegionStub(void);
void SaveRegion(uint8_t *region, int16_t slot, int8_t unload);
void LoadRegionMap(uint8_t *region, void *buf);
void LoadRegion(uint8_t *region, int16_t slot);
void EmptyRegionHook(uint8_t *region, void *buf);

extern uint8_t ReloadingTerrain;
extern uint8_t unused_global_4;
void ReloadRegionTerrain(void);

#endif
