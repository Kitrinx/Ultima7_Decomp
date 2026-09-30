/* Black Gate U7.EXE, resident segment 50 (file offsets 0x020d97 to 0x020edb, 324 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Original folder unknown.
 */

#include "u7port.h"
#include "systimer.h"

struct Stopwatch {
	int8_t running;
	int32_t startTime;
	int32_t total;
	Stopwatch() { total = startTime = 0; }
	void start() { total = 0; startTime = TickCount; running = 1; }
};

uint32_t FrameTimeTotal = 0;
uint32_t FrameCount = 0;

void ResetFrameRate(void)
{
	FrameTimeTotal = 0;
	FrameCount = 0;
}

/* Measures the frame rate; the rates are worked out but never shown. */
void MeasureFrameRate(void)
{
	static Stopwatch stopwatch;
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
