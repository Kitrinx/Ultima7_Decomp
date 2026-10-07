#ifndef WORLDGMP_H
#define WORLDGMP_H

#include "gumps.h"

/* The game world as a place to pick items up from and drop them on. */
struct WorldGump : Panel {
	objref obj;
	int16_t sx, sy, ox, oy;
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
	objref object();
	uint8_t accepts(objref, int16_t, int16_t);
	objref selected();
	int16_t mouseX();
	int16_t mouseY();
	int16_t dragX();
	int16_t dragY();
	void setDragX(int16_t);
	void setDragY(int16_t);
	void refresh(int8_t) {}
};

#endif
