/* Serpent Isle SI.EXE, resident segment 30 (file offsets 0x01972c to 0x019922, 502 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "systimer.h"
#include "speed.h"


unsigned long FrameTimeTotal = 0;
unsigned long FrameCount = 0;
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

unsigned char ToggleFrameTiming(void)
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
				;
		FrameTimer.start();
	}
}

/* Measures the frame rate; the rates are worked out but never shown. */
void MeasureFrameRate(void)
{
	static Stopwatch stopwatch;
	unsigned long elapsed;
	unsigned long unusedRate, unusedRemainder, unusedAverage;

	if ((elapsed = Stopwatch_getElapsed(&stopwatch)) > 0) {
		unusedRate = 0;
		unusedRemainder = 0;
		FrameCount = FrameCount + 1;
		if (FrameCount > 5) {
			FrameTimeTotal = FrameTimeTotal + elapsed;
			unusedAverage = 6000L / (FrameTimeTotal * 10 / (FrameCount - 5));
			unusedRate = 60L / elapsed;
			unusedRemainder = 60L % elapsed;
		}
	}
	stopwatch.start();
}
