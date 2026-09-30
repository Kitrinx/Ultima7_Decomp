#ifndef SHARED_FADEPAL_H
#define SHARED_FADEPAL_H

#include "rgbpal.h"

namespace Shared {

/* A palette that keeps its fade target, so a key press can cut a fade short. */
struct FadingPalette : RgbPalette {
	RgbPalette target;
	FadingPalette() : RgbPalette() {}
	void fillCopy(RgbColor *color, int16_t first, int16_t last);
	void fillSaving(RgbColor *color, int16_t first, int16_t last);
	void fadeToColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks);
	void fadeFromColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks);
};

/* What a key that cuts a fade short runs; it must read the key. Each program sets its own.
 * Without one, the key is read and dropped. */
void SetKeyHandler(void (*handler)(void));

}

#endif
