/* Black Gate U7.EXE, resident segment 50 (file offsets 0x020d97 to 0x020edb, 324 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "systimer.h"

struct Stopwatch {
	char running;
	long startTime;
	long total;
	Stopwatch() { total = startTime = 0; }
	void start() { total = 0; startTime = TickCount; running = 1; }
};

unsigned long FrameTimeTotal = 0;
unsigned long FrameCount = 0;

void ResetFrameRate(void)
{
	FrameTimeTotal = 0;
	FrameCount = 0;
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
