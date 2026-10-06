#ifndef CAMERA_H
#define CAMERA_H

#include "mapview.h"

/* Needs objref declared first. */
struct Coord;
struct CellCoord;
struct ItemId;

/* What the world view follows: an item, or a fixed spot while locked. */
struct Camera {
	unsigned char waitForKey, drawOverlay, locked;
	objref target;
	Camera() { drawOverlay = 0; waitForKey = 0; }
	void setOverlay(unsigned char enabled);
	Coord getCenterX();
	Coord getCenterY();
	int isOnScreen(Coord x, Coord y);
	void toWorldCoords(int x, int y, Coord *outX, Coord *outY);
	void moveTo(Coord x, Coord y);
	void moveToItem(Coord x, Coord y, ItemId item);
	void beginFrame();
	void render();
	void setTarget(objref item);
};

extern Camera gCamera;
extern WorldView MainWorldView;
extern char FirstFramePending;
extern char CopyingFrame;

void far DrawCeilingMask(void);
void far CopyFrameBuffer(void);
void far FinishFrame(char advance);
int far IsWorldPosOnScreen(Coord a, Coord b);
void far Camera_drawAt(Camera *self, CellCoord x, CellCoord y, unsigned char z);
void far Camera_drawAt(Camera *self, Coord x, Coord y, ItemId item);
void far DrawWorld(Camera *self);
void far InitCamera(int);
void far InitChunkCache(void);
void InitMap(void);
void far CenterOnAvatar(void);

#endif
