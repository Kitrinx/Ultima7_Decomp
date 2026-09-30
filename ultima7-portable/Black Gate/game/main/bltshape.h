#ifndef BLTSHAPE_H
#define BLTSHAPE_H

class ShapeManager;
struct objref;
struct View;

extern char *ShapesFileName;
extern char *XformFileName;
extern int16_t DrawTranslation;
extern View *DrawTarget;
extern int16_t DrawType, DrawFrameNumber, DrawCellX, DrawCellY;
extern int8_t DrawMirrored, DrawTranslucent;
extern int16_t DrawZOffset;

void SetDrawTargetScreen();
void SetDrawTargetViewport();
int16_t ShapeManager_getFrameCount(ShapeManager *shapes, int16_t shape);
void ShapeManager_reloadFrame(ShapeManager *shapes, int16_t frame);
void ShapeManager_preload(ShapeManager *shapes, int16_t percent);
void ShapeManager_drawCurrent(ShapeManager *shapes);
void ShapeManager_drawAtCell(ShapeManager *shapes, int16_t shape, int16_t frame, int16_t x, int16_t y, int8_t mirrored);
void ShapeManager_draw(ShapeManager *shapes, View *destination, int16_t x, int16_t y, int16_t shape, int16_t frame,
	int8_t mirrored, int16_t translation);
void ShapeManager_drawInViewport(ShapeManager *shapes, int16_t x, int16_t y, int16_t shape, int16_t frame, int8_t mirrored,
	int16_t translation);
void ShapeManager_drawItem(ShapeManager *shapes, int16_t x, int16_t y, objref item, View *destination);

#endif
