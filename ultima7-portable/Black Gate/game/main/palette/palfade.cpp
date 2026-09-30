/* Black Gate U7.EXE, resident segment 108 (file offsets 0x037b09 to 0x037e6d, 868 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include "plat.h"
#include "lowlevel.h"
#include "u7event.h"
#include "systimer.h"
#include "crawpal.h"
#include "gtimer.h"
#include "worldpal.h"
#include "palfade.h"

/* a stopwatch that can be stopped and restarted */
struct Stopwatch {
	int8_t running;
	uint32_t start;
	uint32_t total;
	Stopwatch() { total = start = 0; }
	void restart() { total = 0; start = TickCount; running = 1; }
};

uint8_t PlayerActionSuspended;

Stopwatch PaletteStopwatch;

/* rotate the cycling colors, a quarter of a second apart */
void CyclePalette(void)
{
	static int16_t unusedCount = 0;
	static int16_t started = 0;
	uint32_t elapsed;

	if (!GameTime.running())
		return;
	unusedCount++;
	if (!started) {
		PaletteStopwatch.restart();
		started = 1;
	}
	elapsed = Stopwatch_getElapsed(&PaletteStopwatch);
	if (elapsed > 15) {
		GameScreen.update();
		if (GameScreen.fading == 0) {
			if (GameScreen.pal->changed() != 0)
				GameScreen.pal->apply(GameScreen.paletteMode);
			else
				SetPaletteRange(GameScreen.pal->colors, GameScreen.pal->order, 224, 31);
		}
		Stopwatch_stop(&PaletteStopwatch);
		PaletteStopwatch.restart();
		unusedCount = 0;
	}
}

/* fade the screen out, one step every ticks / 12 ticks, then hold for a second */
void FadeScreenOut(int16_t ticks, int16_t)
{
	static int16_t unusedCount = 0;
	static int16_t started = 0;
	int16_t step;
	uint32_t elapsed;

	step = ticks / 12;
	if (step > 0) {
		GameScreen.lightBand = 0;
		unusedCount++;
		if (!started) {
			PaletteStopwatch.restart();
			started = 1;
		}
		GameScreen.fadeOut();
		while (GameScreen.fadeSteps != 0) {
			plat_yield();
			elapsed = Stopwatch_getElapsed(&PaletteStopwatch);
			if (step < elapsed) {
				Creeper_runEffect(&GameScreen, EFFECT_FADE_OUT, GameScreen.fadeSteps, 12 - GameScreen.fadeSteps);
				if (--GameScreen.fadeSteps == 0) {
					GameScreen.fading = 0;
					GameScreen.fadeType = 0;
				}
				Stopwatch_stop(&PaletteStopwatch);
				PaletteStopwatch.restart();
				unusedCount = 0;
			}
		}
	}
	PaletteStopwatch.restart();
	while ((elapsed = Stopwatch_getElapsed(&PaletteStopwatch)) < 60)
		plat_yield();
	Stopwatch_stop(&PaletteStopwatch);
	PaletteStopwatch.restart();
	PlayerActionSuspended = 1;
}

/* fade the screen back in, one step every ticks / 12 ticks */
void FadeScreenIn(int16_t ticks, int16_t)
{
	static int16_t unusedCount = 0;
	static int16_t started = 0;
	int16_t step;
	uint32_t elapsed;

	step = ticks / 12;
	GameScreen.selectCurrent();
	if (step > 0) {
		GameScreen.lightBand = 0;
		unusedCount++;
		if (!started) {
			PaletteStopwatch.restart();
			started = 1;
		}
		GameScreen.fadeIn();
		while (GameScreen.fadeSteps != 0) {
			plat_yield();
			elapsed = Stopwatch_getElapsed(&PaletteStopwatch);
			if (step < elapsed) {
				Creeper_runEffect(&GameScreen, EFFECT_FADE_IN, GameScreen.fadeSteps, 12 - GameScreen.fadeSteps);
				if (--GameScreen.fadeSteps == 0) {
					GameScreen.fading = 0;
					GameScreen.fadeType = 0;
				}
				Stopwatch_stop(&PaletteStopwatch);
				PaletteStopwatch.restart();
				unusedCount = 0;
			}
		}
	}
	PlayerActionSuspended = 0;
	FlushKeyboard();
}
