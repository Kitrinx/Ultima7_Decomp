#ifndef BLTSHAPE_H
#define BLTSHAPE_H

class ShapeManager;
struct objref;
struct View;

extern char *ShapesFileName;
extern char *XformFileName;
extern int DrawTranslation;
extern View *DrawTarget;
extern int DrawType, DrawFrameNumber, DrawCellX, DrawCellY;
extern char DrawMirrored, DrawTranslucent;
extern int DrawZOffset;

void far SetDrawTargetScreen();
void far SetDrawTargetViewport();
int far ShapeManager_getFrameCount(ShapeManager *shapes, int shape);
void far ShapeManager_reloadFrame(ShapeManager *shapes, int frame);
void far ShapeManager_preload(ShapeManager *shapes, int percent);
void far ShapeManager_drawCurrent(ShapeManager *shapes);
void far ShapeManager_drawAtCell(ShapeManager *shapes, int shape, int frame, int x, int y, char mirrored);
void far ShapeManager_draw(ShapeManager *shapes, View *destination, int x, int y, int shape, int frame,
	char mirrored, int translation);
void far ShapeManager_drawInViewport(ShapeManager *shapes, int x, int y, int shape, int frame, char mirrored,
	int translation);
void far ShapeManager_drawItem(ShapeManager *shapes, int x, int y, objref item, View *destination);

#endif
