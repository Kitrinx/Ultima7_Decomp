#ifndef SIGNGUMP_H
#define SIGNGUMP_H

#include "gumps.h"
#include "itable.h"

/* Up to ten lines of text, split at carriage returns, drawn in a proportional font. */
struct SignGump : Control {
	int x, y, lineCount, shape, style;
	char lines[10][40];
	ProportionalTextPrinter printer;
	SignGump(int, char *);
	void moveTo(int, int);
	unsigned char handle(MouseState *);
	void draw(View *);
};

#endif
