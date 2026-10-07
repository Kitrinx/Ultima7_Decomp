/* Serpent Isle MAINMENU.EXE, resident segment 25 (file offsets 0x0101e1 to 0x0107f9, 1560 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <stdlib.h>
#include "plat.h"
#include "chkfile.h"
#include "flex.h"
#include "vidmode.h"
#include "rgbpal.h"

namespace Shared {

/* What the DAC was last given. */
static RgbColor Dac[PALETTE_COLORS];

static void SetDacRange(int16_t first, int16_t count, RgbColor *colors)
{
	uint8_t rgb[PALETTE_COLORS * 3];
	int16_t i;

	for (i = 0; i < count; i++) {
		Dac[first + i].red = colors[i].red & 0x3f;
		Dac[first + i].green = colors[i].green & 0x3f;
		Dac[first + i].blue = colors[i].blue & 0x3f;
		rgb[i * 3] = Dac[first + i].red;
		rgb[i * 3 + 1] = Dac[first + i].green;
		rgb[i * 3 + 2] = Dac[first + i].blue;
	}
	plat_video_set_palette(first, count, rgb);
}

void RgbColor::set(RgbColor *color)
{
	red = color->red;
	green = color->green;
	blue = color->blue;
}

void CopyColors(RgbColor *colors, RgbPalette *palette)
{
	int16_t count = PALETTE_COLORS;
	RgbColor *to = colors;
	RgbColor *from = palette->getColor(0);

	while (count--)
		(to++)->set(from++);
}

/* Loads DAC registers 0 to 254, as the BIOS call did. */
void SetVgaPalette(RgbColor *colors)
{
	SetDacRange(0, PALETTE_COLORS - 1, colors);
}

void SetDacColor(int16_t index, RgbColor *color)
{
	SetDacRange(index, 1, color);
}

void GetDacColor(int16_t index, RgbColor *color)
{
	*color = Dac[index];
}

/* Steps each level one toward zero; returns 1 once all are zero. */
uint8_t DimColors(uint8_t *level)
{
	uint8_t dark = 1;
	int16_t i;

	for (i = 0; i < 256; i++, level++)
		if (*level != 0) {
			dark = 0;
			(*level)--;
		}
	return dark;
}

void CopyColor(RgbColor *to, RgbColor *from)
{
	to->red = from->red;
	to->green = from->green;
	to->blue = from->blue;
}

void RgbPalette::randomize()
{
	int16_t i;

	for (i = 0; i < PALETTE_COLORS; i++) {
		colors[i].red = random(64);
		colors[i].green = random(64);
		colors[i].blue = random(64);
	}
}

/* Reads the palette the card is showing. */
void RgbPalette::capture()
{
	RgbColor *color = colors;
	int16_t i;

	for (i = 0; i < PALETTE_COLORS; i++, color++)
		GetDacColor(i, color);
}

void RgbPalette::apply()
{
	RgbColor *color = colors;
	int16_t i;

	WaitForRetrace();
	for (i = 0; i < PALETTE_COLORS; i++, color++)
		SetDacColor(i, color);
}

void RgbPalette::loadFile(char *name)
{
	DataFile file(name, 1);

	file.read(colors, sizeof colors);
}

void RgbPalette::load(char *flexName, int16_t i)
{
	Flex flex;

	flex.open(flexName);
	FlexEntry entry;
	flex.getEntry(i, &entry);
	if (entry.size > (int32_t) (PALETTE_COLORS * 3 * sizeof(int16_t)))
		entry.size = PALETTE_COLORS * 3 * sizeof(int16_t);
	flex.readEntry(&entry, colors, 0);
	flex.close();
}

void RgbPalette::saveFile(char *name)
{
	DataFile file(name, 0);

	file.write(colors, sizeof colors);
}

/* Sets colors first to last to one color. */
void RgbPalette::fill(RgbColor *color, int16_t first, int16_t last)
{
	for (; first <= last; first++) {
		colors[first].red = color->red;
		colors[first].green = color->green;
		colors[first].blue = color->blue;
	}
}

RgbPalette &RgbPalette::operator=(RgbPalette &palette)
{
	int16_t i;

	for (i = 0; i < PALETTE_COLORS; i++) {
		colors[i].red = palette.getColor(i)->red;
		colors[i].green = palette.getColor(i)->green;
		colors[i].blue = palette.getColor(i)->blue;
	}
	return *this;
}

int16_t RgbPalette::fadeStep(RgbPalette *unused)
{
	WaitForRetrace();
	return fade.advance();
}

/* Copies this palette into copy, then fills copy's first to last with color. */
void RgbPalette::fillCopy(RgbColor *color, RgbPalette *copy, int16_t first, int16_t last)
{
	*copy = *this;
	copy->fill(color, first, last);
}

/* Saves this palette into saved, then fills first to last with color. */
void RgbPalette::fillSaving(RgbColor *color, RgbPalette *saved, int16_t first, int16_t last)
{
	*saved = *this;
	fill(color, first, last);
}

/* Fades first to last to color, delay retraces a step, over ticks steps. */
void RgbPalette::fadeToColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks)
{
	RgbPalette target;
	int16_t wait;

	fadeTicks = ticks;
	fillCopy(color, &target, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
}

/* Fills first to last with color, then fades back to the palette as it was. */
void RgbPalette::fadeFromColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks)
{
	RgbPalette target;
	int16_t wait;

	fadeTicks = ticks;
	fillSaving(color, &target, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
}

/* Moves colors first to last one place toward first, wrapping first round to last; shows them if asked. */
void RgbPalette::rotate(int16_t first, int16_t last, uint8_t show)
{
	RgbColor saved;
	int16_t i;

	if (first < last) {
		CopyColor(&saved, &colors[first]);
		for (i = first; i < last; i++)
			CopyColor(&colors[i], &colors[i + 1]);
		CopyColor(&colors[last], &saved);
	} else {
		CopyColor(&saved, &colors[first]);
		for (i = first; i > last; i--)
			CopyColor(&colors[i], &colors[i - 1]);
		CopyColor(&colors[last], &saved);
		i = first;
		first = last;
		last = i;
	}
	if (show) {
		WaitForRetrace();
		for (i = first; i <= last; i++)
			SetDacColor(i, &colors[i]);
	}
}

}

extern "C" void ResetSharedRgbpalGlobals(void)
{
	memset(Shared::Dac, 0, sizeof Shared::Dac);
}
