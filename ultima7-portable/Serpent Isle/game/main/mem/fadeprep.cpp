/* Palette conversion and fade set-up over 768 components in linear memory.
 * Words hold a component in their high byte. */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"

#define PALETTE_SIZE 768

void PaletteWordsToBytes(int32_t dest, int32_t src)
{
	int16_t i;

	for (i = 0; i < PALETTE_SIZE; i++)
		LinearPut8(dest + i, (uint8_t) (LinearGet16(src + i * 2) >> 8));
}

void PaletteBytesToWords(int32_t dest, int32_t src)
{
	int16_t i;

	for (i = 0; i < PALETTE_SIZE; i++)
		LinearPut16(dest + i * 2, (uint16_t) (LinearGet8(src + i) << 8));
}

/* Stores each gap between the current words and the target bytes, and that gap over steps
 * rounded away from zero. Returns steps. */
int16_t PreparePaletteFade(int32_t current, int32_t target, int32_t gaps, int32_t deltas, int16_t steps)
{
	int16_t i, gap, delta, rest;

	for (i = 0; i < PALETTE_SIZE; i++) {
		gap = (int16_t) (LinearGet16(current + i * 2) - (LinearGet8(target + i) << 8));
		LinearPut16(gaps + i * 2, (uint16_t) gap);
		delta = 0;
		rest = 0;
		if (gap != 0) {
			delta = gap / steps;
			rest = gap % steps;
		}
		if (rest != 0) {
			if (delta < 0 || (delta == 0 && rest < 0))
				delta--;
			else
				delta++;
		}
		LinearPut16(deltas + i * 2, (uint16_t) delta);
	}
	return steps;
}
