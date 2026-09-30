#ifndef CAMERA_H
#define CAMERA_H

#include "mapview.h"

/* Needs objref declared first. */
struct Coord;
struct CellCoord;
struct ItemId;

/* What the world view follows: an item, or a fixed spot while locked. */
struct Camera {
	uint8_t waitForKey, drawOverlay, locked;
	objref target;
	Camera() { drawOverlay = 0; waitForKey = 0; }
	void setOverlay(uint8_t enabled);
	Coord getCenterX();
	Coord getCenterY();
	int16_t isOnScreen(Coord x, Coord y);
	void toWorldCoords(int16_t x, int16_t y, Coord *outX, Coord *outY);
	void moveTo(Coord x, Coord y);
	void moveToItem(Coord x, Coord y, ItemId item);
	void beginFrame();
	void render();
	void setTarget(objref item);
};

extern Camera gCamera;
extern WorldView MainWorldView;
extern int8_t FirstFramePending;
extern int8_t CopyingFrame;

void DrawCeilingMask(void);
void CopyFrameBuffer(void);
void FinishFrame(int8_t advance);
int16_t IsWorldPosOnScreen(Coord a, Coord b);
void Camera_drawAt(Camera *self, CellCoord x, CellCoord y, uint8_t z);
void Camera_drawAt(Camera *self, Coord x, Coord y, ItemId item);
void DrawWorld(Camera *self);
void InitCamera(int16_t);
void InitChunkCache(void);
void InitMap(void);
void CenterOnAvatar(void);

#endif
