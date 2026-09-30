#ifndef TOOLS_H
#define TOOLS_H

extern int16_t unused_global_2;
extern char *WorldMapFileName;

uint8_t IsPointOnMap(int16_t x, int16_t y);
void DrawWorldMap(int16_t check, int16_t *shape);
void TeleportByMap(void);
void ShowWorldMap(void);
void RunWizardEye(int16_t count);
void CheckItemBuffer(void);

#endif
