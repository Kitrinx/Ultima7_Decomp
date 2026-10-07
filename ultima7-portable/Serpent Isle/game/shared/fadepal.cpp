/* Serpent Isle MAINMENU.EXE, resident segment 26 (file offsets 0x0107f9 to 0x010990, 407 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "vidmode.h"
#include "keys.h"
#include "fadepal.h"

namespace MainMenu {
void HandleKey(void);
}

namespace Shared {

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

/* Fades first to last to color; a key press jumps to the end. */
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
		MainMenu::HandleKey();
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
		MainMenu::HandleKey();
		while (fadeStep(&target))
			;
		apply();
	}
}

}
