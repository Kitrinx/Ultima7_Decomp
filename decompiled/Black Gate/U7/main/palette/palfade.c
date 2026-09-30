/* Black Gate U7.EXE, resident segment 108 (file offsets 0x037b09 to 0x037e6d, 868 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "lowlevel.h"
#include "u7event.h"
#include "systimer.h"
#include "crawpal.h"
#include "gtimer.h"
#include "worldpal.h"
#include "palfade.h"

/* a stopwatch that can be stopped and restarted */
struct Stopwatch {
	char running;
	unsigned long start;
	unsigned long total;
	Stopwatch() { total = start = 0; }
	void restart() { total = 0; start = TickCount; running = 1; }
};

unsigned char PlayerActionSuspended;

Stopwatch PaletteStopwatch;

/* rotate the cycling colors, a quarter of a second apart */
void far CyclePalette(void)
{
	static int unusedCount = 0;
	static int started = 0;
	unsigned long elapsed;

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
void far FadeScreenOut(int ticks, int)
{
	static int unusedCount = 0;
	static int started = 0;
	int step;
	unsigned long elapsed;

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
		;
	Stopwatch_stop(&PaletteStopwatch);
	PaletteStopwatch.restart();
	PlayerActionSuspended = 1;
}

/* fade the screen back in, one step every ticks / 12 ticks */
void far FadeScreenIn(int ticks, int)
{
	static int unusedCount = 0;
	static int started = 0;
	int step;
	unsigned long elapsed;

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
