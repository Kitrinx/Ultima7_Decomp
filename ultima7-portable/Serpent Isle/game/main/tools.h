#ifndef TOOLS_H
#define TOOLS_H

extern int16_t LowFn;
extern int16_t HighFn;
extern char *const WorldMapFileName;

uint8_t IsPointOnMap(int16_t x, int16_t y);
void DrawWorldMap(int16_t map, int16_t *shape);
void TeleportByMap(void);
void ShowWorldMap(uint8_t map);
void RunWizardEye(int16_t count);

#endif
