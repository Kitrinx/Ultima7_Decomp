/* Serpent Isle MAINMENU.EXE, resident segment 27 (file offsets 0x010990 to 0x010c12, 642 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <math.h>
#include "errors.h"
#include "rgbpal.h"

namespace Shared {

#define FADE_SCALE  0x7fff

/* Sets up a fade of count components from from toward to over ticks steps. */
int16_t Fade::prepare(uint8_t *from, uint8_t *to, int16_t count, int16_t ticks, int16_t unused)
{
	int16_t i;

	size = count;
	colors = from;
	target = to;
	if (accum == 0)
		accum = new uint16_t[size];
	if (period == 0)
		period = new uint16_t[size];
	if (step == 0)
		step = new int16_t[size];
	if (active == 0)
		active = new uint8_t[size];
	if (accum == 0 || period == 0 || step == 0 || active == 0) {
		delete[] accum;
		delete[] period;
		delete[] step;
		delete[] active;
		FatalError("Couldn't allocate accumulator memory.");
		return 0;
	}
	rate = FADE_SCALE / ticks;
	steps = ticks;
	for (i = 0; i < size; i++) {
		active[i] = 1;
		step[i] = target[i] - colors[i];
		if (step[i] != 0) {
			period[i] = FADE_SCALE / abs(step[i]);
			accum[i] = 0;
			if (step[i] > 0)
				step[i] = 1;
			else
				step[i] = -1;
		} else
			active[i] = 0;
	}
	return 1;
}

/* Moves the fade one tick; on the last one, lands every component on its target. */
int16_t Fade::advance()
{
	int16_t i;

	if (steps-- != 0) {
		for (i = 0; i < size; i++) {
			if (!active[i])
				continue;
			accum[i] += rate;
			while (accum[i] >= period[i]) {
				accum[i] -= period[i];
				colors[i] += step[i];
			}
		}
		return 1;
	}
	for (i = 0; i < size; i++)
		colors[i] = target[i];
	return 0;
}

/* Frees the work arrays. */
void Fade::release()
{
	if (accum) {
		delete[] accum;
		accum = 0;
	}
	if (period) {
		delete[] period;
		period = 0;
	}
	if (step) {
		delete[] step;
		step = 0;
	}
	if (active) {
		delete[] active;
		active = 0;
	}
}

}
