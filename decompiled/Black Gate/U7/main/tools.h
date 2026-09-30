#ifndef TOOLS_H
#define TOOLS_H

extern int unused_global_2;
extern char *WorldMapFileName;

unsigned char IsPointOnMap(int x, int y);
void DrawWorldMap(int check, int *shape);
void TeleportByMap(void);
void ShowWorldMap(void);
void RunWizardEye(int count);
void CheckItemBuffer(void);

#endif
