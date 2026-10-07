/* Serpent Isle SI.EXE, resident segment 30 (file offsets 0x01972c to 0x019922, 502 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "systimer.h"
#include "speed.h"


uint32_t FrameTimeTotal = 0;
uint32_t FrameCount = 0;
Stopwatch FrameTimer;

void ResetFrameRate(void)
{
	FrameTimeTotal = 0;
	FrameCount = 0;
}

void ResetFrameTiming(void)
{
	FrameTimer.start();
}

uint8_t ToggleFrameTiming(void)
{
	if (FrameTimer.running) {
		Stopwatch_stop(&FrameTimer);
		return 0;
	}
	ResetFrameTiming();
	return 1;
}

void WaitForFrame(void)
{
	if (FrameTimer.running) {
		if (Stopwatch_getElapsed(&FrameTimer) >= 0)
			while (Stopwatch_getElapsed(&FrameTimer) < 6)
				plat_yield();
		FrameTimer.start();
	}
}

static Stopwatch stopwatch;

/* Measures the frame rate; the rates are worked out but never shown. */
void MeasureFrameRate(void)
{
	uint32_t elapsed;
	uint32_t unusedRate, unusedRemainder, unusedAverage;

	if ((elapsed = Stopwatch_getElapsed(&stopwatch)) > 0) {
		unusedRate = 0;
		unusedRemainder = 0;
		FrameCount = FrameCount + 1;
		if (FrameCount > 5) {
			FrameTimeTotal = FrameTimeTotal + elapsed;
			unusedAverage = INT32_C(6000) / (FrameTimeTotal * 10 / (FrameCount - 5));
			unusedRate = INT32_C(60) / elapsed;
			unusedRemainder = INT32_C(60) % elapsed;
		}
	}
	stopwatch.start();
}

extern "C" void ResetSpeedGlobals(void)
{
	FrameTimeTotal = 0;
	FrameCount = 0;
	memset((void *)&FrameTimer, 0, sizeof(FrameTimer));
	memset((void *)&stopwatch, 0, sizeof(stopwatch));
}

extern "C" void ConstructSpeedGlobals(void)
{
	new (&FrameTimer) Stopwatch();
	new (&stopwatch) Stopwatch();
}
