#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

#define PALETTE_SIZE 768

/* Moves 768 fading components one step, never past the remaining gap; a component whose
 * gap runs out stops. A positive gap compares unsigned, a negative one signed. */
void StepPaletteFade(int32_t colors, int32_t gaps, int32_t deltas)
{
	int16_t i;
	uint16_t color, gap, delta;

	for (i = 0; i < PALETTE_SIZE; i++, colors += 2, gaps += 2, deltas += 2) {
		delta = LinearGet16(deltas);
		if (delta == 0)
			continue;
		color = LinearGet16(colors);
		gap = LinearGet16(gaps);
		if ((int16_t) gap >= 0 ? delta >= gap : (int16_t) delta < (int16_t) gap) {
			color -= gap;
			gap = 0;
			LinearPut16(deltas, 0);
		} else {
			color -= delta;
			gap -= delta;
		}
		LinearPut16(gaps, gap);
		LinearPut16(colors, color);
	}
}
