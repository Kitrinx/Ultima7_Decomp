/* Black Gate U7.EXE, resident segment 108 (file offsets 0x037b09 to 0x037e6d, 868 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <new>
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
static int16_t CycleUnusedCount = 0;
static int16_t CycleStarted = 0;

void CyclePalette(void)
{
	uint32_t elapsed;

	if (!GameTime.running())
		return;
	CycleUnusedCount++;
	if (!CycleStarted) {
		PaletteStopwatch.restart();
		CycleStarted = 1;
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
		CycleUnusedCount = 0;
	}
}

/* fade the screen out, one step every ticks / 12 ticks, then hold for a second */
static int16_t FadeOutUnusedCount = 0;
static int16_t FadeOutStarted = 0;

void FadeScreenOut(int16_t ticks, int16_t)
{
	int16_t step;
	uint32_t elapsed;

	step = ticks / 12;
	if (step > 0) {
		GameScreen.lightBand = 0;
		FadeOutUnusedCount++;
		if (!FadeOutStarted) {
			PaletteStopwatch.restart();
			FadeOutStarted = 1;
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
				FadeOutUnusedCount = 0;
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
static int16_t FadeInUnusedCount = 0;
static int16_t FadeInStarted = 0;

void FadeScreenIn(int16_t ticks, int16_t)
{
	int16_t step;
	uint32_t elapsed;

	step = ticks / 12;
	GameScreen.selectCurrent();
	if (step > 0) {
		GameScreen.lightBand = 0;
		FadeInUnusedCount++;
		if (!FadeInStarted) {
			PaletteStopwatch.restart();
			FadeInStarted = 1;
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
				FadeInUnusedCount = 0;
			}
		}
	}
	PlayerActionSuspended = 0;
	FlushKeyboard();
}

extern "C" void ResetPalfadeGlobals(void)
{
	PlayerActionSuspended = 0;
	memset((void *)&PaletteStopwatch, 0, sizeof(PaletteStopwatch));
	CycleUnusedCount = 0;
	CycleStarted = 0;
	FadeOutUnusedCount = 0;
	FadeOutStarted = 0;
	FadeInUnusedCount = 0;
	FadeInStarted = 0;
}

extern "C" void ConstructPalfadeGlobals(void)
{
	new (&PaletteStopwatch) Stopwatch();
}
