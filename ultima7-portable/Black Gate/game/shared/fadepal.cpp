/* Black Gate shared module FADEPAL, linked into MAINMENU.EXE and INTRO.EXE: palette fades a key
 * press can cut short.
 */

#include "u7port.h"
#include "vidmode.h"
#include "keys.h"
#include "fadepal.h"

namespace Shared {

static void DropKey(void)
{
	if (GetKey() == 0)
		GetKey();
}

static void (*KeyHandler)(void) = DropKey;

void SetKeyHandler(void (*handler)(void))
{
	KeyHandler = handler ? handler : DropKey;
}

/* Makes the target this palette with first to last set to color. */
void FadingPalette::fillCopy(RgbColor *color, int16_t first, int16_t last)
{
	RgbPalette::fillCopy(color, &target, first, last);
}

/* Saves this palette as the target, then sets first to last to color. */
void FadingPalette::fillSaving(RgbColor *color, int16_t first, int16_t last)
{
	RgbPalette::fillSaving(color, &target, first, last);
}

/* Fades first to last to color; a key press jumps to the end. The delay is ignored. */
void FadingPalette::fadeToColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks)
{
	int16_t wait;

	delay = 0;
	fadeTicks = ticks;
	fillCopy(color, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance() && !KeyHit()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
	if (KeyHit()) {
		KeyHandler();
		while (fadeStep(&target))
			;
		apply();
	}
}

/* Sets first to last to color, then fades back to the palette as it was; a key press jumps to the end. */
void FadingPalette::fadeFromColor(RgbColor *color, int16_t delay, int16_t first, int16_t last, int16_t ticks)
{
	int16_t wait;

	fadeTicks = ticks;
	fillSaving(color, first, last);
	fade.prepare(&colors[0].red, &target.colors[0].red, sizeof colors, fadeTicks, 0);
	while (fade.advance() && !KeyHit()) {
		if (delay > 1) {
			wait = delay - 1;
			while (wait--)
				WaitForRetrace();
		}
		apply();
	}
	if (KeyHit()) {
		KeyHandler();
		while (fadeStep(&target))
			;
		apply();
	}
}

}

extern "C" void ResetSharedFadepalGlobals(void)
{
	Shared::KeyHandler = Shared::DropKey;
}
