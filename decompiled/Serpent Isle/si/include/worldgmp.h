#ifndef WORLDGMP_H
#define WORLDGMP_H

#include "gumps.h"

/* The game world as a place to pick items up from and drop them on. */
struct WorldGump : Panel {
	objref obj;
	int sx, sy, ox, oy;
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
	objref object();
	unsigned char accepts(objref, int, int);
	objref selected();
	int mouseX();
	int mouseY();
	int dragX();
	int dragY();
	void setDragX(int);
	void setDragY(int);
	void refresh(char) {}
};

#endif
