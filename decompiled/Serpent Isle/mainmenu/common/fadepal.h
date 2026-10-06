#ifndef FADEPAL_H
#define FADEPAL_H

#include "rgbpal.h"

/* A palette that keeps its fade target, so a key press can cut a fade short. */
struct FadingPalette : RgbPalette {
	RgbPalette target;
	FadingPalette() : RgbPalette() {}
	void fillCopy(RgbColor *color, int first, int last);
	void fillSaving(RgbColor *color, int first, int last);
	void fadeToColor(RgbColor *color, int delay, int first, int last, int ticks);
	void fadeFromColor(RgbColor *color, int delay, int first, int last, int ticks);
};

#endif
