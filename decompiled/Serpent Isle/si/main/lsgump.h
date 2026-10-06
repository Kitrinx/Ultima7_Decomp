#ifndef LSGUMP_H
#define LSGUMP_H

#include "gumps.h"

/* A save slot's editable name. */
struct SaveSlot : CounterSprite {
	char text[80];
	SaveSlot(int n) : CounterSprite(n) {}
	void draw(View *);
	void edit(unsigned key, unsigned char replace);
};

/* The yes or no question box. */
struct YesNoGump : Sprite {
	SizedSprite yes, no;
	Sprite message;
	YesNoGump(int);
	unsigned char handle(MouseState *);
	void moveTo(int, int);
	void draw(View *);
};

unsigned char far DoYesNoDialog(int);

extern ProportionalTextPrinter SaveSlotTextPrinter;

#endif
