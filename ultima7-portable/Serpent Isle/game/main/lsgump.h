#ifndef LSGUMP_H
#define LSGUMP_H

#include "gumps.h"

/* A save slot's editable name. */
struct SaveSlot : CounterSprite {
	char text[80];
	SaveSlot(int16_t n) : CounterSprite(n) {}
	void draw(View *);
	void edit(uint16_t key, uint8_t replace);
};

/* The yes or no question box. */
struct YesNoGump : Sprite {
	SizedSprite yes, no;
	Sprite message;
	YesNoGump(int16_t);
	uint8_t handle(MouseState *);
	void moveTo(int16_t, int16_t);
	void draw(View *);
};

uint8_t DoYesNoDialog(int16_t);

extern ProportionalTextPrinter SaveSlotTextPrinter;

#endif
