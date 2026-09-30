#ifndef SIGNGUMP_H
#define SIGNGUMP_H

#include "gumps.h"
#include "itable.h"

/* Up to ten lines of text, split at carriage returns, drawn in a proportional font. */
struct SignGump : Control {
	int16_t x, y, lineCount, shape, style;
	char lines[10][40];
	ProportionalTextPrinter printer;
	SignGump(int16_t, char *);
	void moveTo(int16_t, int16_t);
	uint8_t handle(MouseState *);
	void draw(View *);
};

#endif
