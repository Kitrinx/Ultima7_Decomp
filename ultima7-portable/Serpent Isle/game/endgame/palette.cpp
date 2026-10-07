/* Black Gate ENDGAME.EXE: palette.c, palstep.c and dacwrite.asm. The DAC is the platform's
 * palette, and the retrace wait the platform's.
 */

#include "u7port.h"
#include "plat.h"
#include "palette.h"

namespace Endgame {

static inline int16_t Smaller(int16_t a, int16_t b) { return a < b ? a : b; }

/* How long a 386DX-33 spent on one step of a fade that did not wait for the retrace. */
#define FADE_STEP_MICROSECONDS 3240

/* Waits out one such step; due is when the last one ended, in microseconds. */
static uint32_t SpendFadeStep(uint32_t due)
{
	due += FADE_STEP_MICROSECONDS;
	while ((int32_t) (plat_milliseconds() * 1000 - due) < 0)
		plat_yield();
	return due;
}

void WriteDac(uint8_t first, int16_t count, Rgb *from)
{
	plat_video_set_palette(first, count, (const uint8_t *) from);
}

static int16_t FadeStarting = 1;
static uint8_t *FadeError;
static uint8_t *FadeDelta;
static int8_t *FadeDirection;
static uint16_t FadeLargest;
static uint16_t FadeStepsLeft;

/* Moves each of count palette components one step closer to its target, spreading the steps so that
 * every component arrives together. Returns 0 once they have all arrived. */
static int16_t StepPaletteComponents(uint8_t *current, uint8_t *target, uint16_t count)
{
	uint16_t i;
	int16_t diff;

	if (FadeStarting) {
		FadeError = new uint8_t[count];
		FadeDelta = new uint8_t[count];
		FadeDirection = new int8_t[count];
		FadeLargest = 0;
		for (i = 0; i < count; i++) {
			diff = current[i] - target[i];
			if (diff < 0) {
				diff = -diff;
				FadeDirection[i] = 1;
			} else
				FadeDirection[i] = -1;
			FadeDelta[i] = (uint8_t) diff;
			if (diff > FadeLargest)
				FadeLargest = diff;
		}
		diff = FadeLargest / 2;
		for (i = 0; i < count; i++)
			FadeError[i] = (uint8_t) diff;
		FadeStepsLeft = FadeLargest;
		FadeStarting = 0;
	}
	if (FadeStepsLeft-- == 0) {
		delete[] FadeError;
		delete[] FadeDelta;
		delete[] FadeDirection;
		FadeStarting = 1;
		return 0;
	}
	for (i = 0; i < count; i++) {
		FadeError[i] += FadeDelta[i];
		if (FadeError[i] > FadeLargest) {
			FadeError[i] -= FadeLargest;
			current[i] += FadeDirection[i];
		}
	}
	return 1;
}

/* Makes room for n entries from first. */
void Palette::init(uint8_t first, int16_t n)
{
	start = first;
	count = n;
	colors = new Rgb[n];
}

void Palette::set(Rgb *from)
{
	for (uint16_t i = 0; i < count; i++)
		colors[i] = from[i];
}

void Palette::load(uint8_t first, int16_t n, Rgb *from)
{
	init(first, n);
	set(from);
}

/* Writes n entries from first to the DAC, batch entries each vertical retrace; all at once when batch
 * is 0. */
void Palette::write(uint8_t first, uint16_t n, uint16_t batch)
{
	Rgb *c;

	if (colors == 0)
		return;
	if (start > first)
		first = start;
	if (start + count < first + n)
		n = start + count - first;
	c = colors + (first - start);
	if (batch) {
		uint16_t size = n / batch;
		uint8_t index = first;
		int16_t left = n;

		if (n % batch)
			size++;
		while (size > 0) {
			plat_video_wait_retrace();
			WriteDac(index, size, c);
			index += size;
			c += size;
			size = Smaller(left = left - size, size);
		}
	} else
		WriteDac(first, n, c);
}

/* Sets n entries from first to one color. */
void Palette::fill(Rgb *color, uint8_t first, uint16_t n)
{
	if (colors) {
		if (start > first)
			first = start;
		if (start + count < first + n)
			n = start + count - first;
		Rgb *c = colors;
		/* moves the run's own pointer; the fill still starts at the old one */
		colors = colors + (first - start);
		for (uint16_t i = 0; i < n; i++, c++)
			*c = *color;
	}
}

/* Copies all of another palette's entries into this one at the other's start; first and n are only
 * clipped. */
void Palette::copy(Palette *from, uint8_t first, uint16_t n)
{
	if (colors == 0 || from->colors == 0)
		return;
	Rgb *s = from->colors;
	Rgb *d = colors + from->start;
	for (uint16_t i = 0; i < from->count; i++)
		*d++ = *s++;
}

/* Writes one entry, if it is in the run. */
void Palette::writeOne(Rgb *color, uint8_t index)
{
	if (start < index && start + count > index)
		WriteDac(index, 1, color);
}

/* Moves every entry one step toward target's. Returns 0 once they are all there. */
uint8_t Palette::step(Palette *target)
{
	uint8_t *current = (uint8_t *) (colors + target->start);
	uint8_t *goal = (uint8_t *) target->colors;

	return (uint8_t) StepPaletteComponents(current, goal, target->count * 3);
}

Palette::~Palette()
{
	if (colors)
		delete[] colors;
}

/* Fades n entries from first to color. */
void Palette::fadeToColor(Rgb *color, uint16_t batch, uint8_t first, uint16_t n)
{
	Rgb *solid = new Rgb[n];

	for (uint16_t i = 0; i < n; i++)
		solid[i] = *color;
	Palette target(first, n, solid);
	delete[] solid;
	uint32_t due = plat_milliseconds() * 1000;
	while (step(&target)) {
		if (batch == 0)
			due = SpendFadeStep(due);
		write(start, count, batch);
	}
}

/* Fades n entries from first in from color to what the palette holds now. */
void Palette::fadeFromColor(Rgb *color, uint16_t batch, uint8_t first, uint16_t n)
{
	Palette target(first, n);

	target.copy(this, start, count);
	fill(color, first, n);
	uint32_t due = plat_milliseconds() * 1000;
	while (step(&target)) {
		if (batch == 0)
			due = SpendFadeStep(due);
		write(start, count, batch);
	}
}

}

extern "C" void ResetEndgamePaletteGlobals(void)
{
	Endgame::FadeStarting = 1;
	Endgame::FadeError = 0;
	Endgame::FadeDelta = 0;
	Endgame::FadeDirection = 0;
	Endgame::FadeLargest = 0;
	Endgame::FadeStepsLeft = 0;
}
