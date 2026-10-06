#ifndef TOOLS_H
#define TOOLS_H

extern int LowFn;
extern int HighFn;
extern char *WorldMapFileName;

unsigned char IsPointOnMap(int x, int y);
void DrawWorldMap(int map, int *shape);
void TeleportByMap(void);
void ShowWorldMap(unsigned char map);
void RunWizardEye(int count);

#endif
