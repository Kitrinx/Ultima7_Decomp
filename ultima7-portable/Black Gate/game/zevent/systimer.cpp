/* Black Gate U7.EXE, resident segment 53 (file offsets 0x020f81 to 0x0212a6, 805 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include "plat.h"
#include "redscrn.h"
#include "systimer.h"

#define TICKS_PER_SECOND 60

/* A stopwatch that can be stopped and restarted. */
struct Stopwatch {
	int8_t running;
	uint32_t start;
	uint32_t total;
};

uint32_t TickCount = 0;        /* ticks, 60 a second */

static uint8_t TickTimerInstalled = 0;
RedScreen RedScreenPicture;
SysTimer SystemTimer;

static void AdvanceTickCount(void)
{
	TickCount++;
}

SysTimer::SysTimer()
{
	TickCount = 0;
}

SysTimer::~SysTimer()
{
	if (TickTimerInstalled) {
		plat_game_timer_remove(AdvanceTickCount);
		TickTimerInstalled = 0;
	}
}

void SysTimer::install()
{
	if (!TickTimerInstalled) {
		plat_game_timer_add(AdvanceTickCount);
		TickTimerInstalled = 1;
	}
}

void Stopwatch_stop(Stopwatch *w)
{
	if (w->running) {
		w->total += TickCount - w->start;
		w->running = 0;
	}
}

uint32_t Stopwatch_getElapsed(Stopwatch *w)
{
	if (w->running)
		return w->total + TickCount - w->start;
	return w->total;
}

void Stopwatch_waitUntil(Stopwatch *w, uint32_t t)
{
	while (t + w->start - TickCount > 0)
		plat_yield();
}

void Timer_wait(Timer *t)
{
	while (t->start + t->length >= TickCount)
		plat_yield();
}

void Timer_restart(Timer *t)
{
	t->start = TickCount;
}

void Timer_set(Timer *t, uint32_t length)
{
	t->length = length;
	t->start = TickCount;
}

void Timer_setLength(Timer *t, uint32_t length)
{
	t->length = length;
}

uint32_t Timer_getElapsed(Timer *t)
{
	return TickCount - t->start;
}

uint32_t Timer_getRemaining(Timer *t)
{
	return t->start + t->length - TickCount;
}

uint32_t Timer_getElapsedSeconds(Timer *t)
{
	return (TickCount - t->start) / TICKS_PER_SECOND;
}

uint32_t Timer_getRemainingSeconds(Timer *t)
{
	return (t->start + t->length - TickCount) / TICKS_PER_SECOND;
}

uint8_t Timer_hasFinished(Timer *t)
{
	return t->start + t->length <= TickCount;
}

void Timer_delay(Timer *t, uint32_t length)
{
	t->start = TickCount;
	t->length = length;
	Timer_wait(t);
}
