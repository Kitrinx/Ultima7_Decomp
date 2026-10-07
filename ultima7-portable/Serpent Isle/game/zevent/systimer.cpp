/* Serpent Isle SI.EXE, resident segment 117 (file offsets 0x03c021 to 0x03c349, 808 bytes).
 * Borland C++ 2.0 -mm -O -G -P -1 -Y rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "redscrn.h"
#include "systimer.h"
#include "init.h"
#include "ail.h"

#define TICKS_PER_SECOND 60


uint8_t TimerSystemInstalled = 0;
uint32_t TickCount = 0;
RedScreen RedScreenPicture;
SysTimer SystemTimer;

SysTimer::SysTimer()
{
	handle = -1;
}

void AdvanceSystemClock(void)
{
	TickCount++;
}

SysTimer::~SysTimer()
{
	if (TimerSystemInstalled) {
		AIL_shutdown("bye!");
		plat_game_timer_remove(AdvanceSystemClock);
		TimerSystemInstalled = 0;
	}
}

void SysTimer::install()
{
	if (!TimerSystemInstalled) {
		AIL_startup();
		plat_game_timer_add(AdvanceSystemClock);
		TimerSystemInstalled = 1;
	}
}

void Stopwatch_stop(Stopwatch *w)
{
	if (w->running) {
		w->total += TickCount - w->startTime;
		w->running = 0;
	}
}

uint32_t Stopwatch_getElapsed(Stopwatch *w)
{
	if (w->running)
		return w->total + TickCount - w->startTime;
	return w->total;
}

void Stopwatch_waitUntil(Stopwatch *w, uint32_t t)
{
	while (t + w->startTime - TickCount > 0)
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

extern "C" void ResetSystimerGlobals(void)
{
	TickCount = 0;
	TimerSystemInstalled = 0;
	memset((void *)&RedScreenPicture, 0, sizeof(RedScreenPicture));
	memset((void *)&SystemTimer, 0, sizeof(SystemTimer));
}

extern "C" void ConstructSystimerGlobals(void)
{
	new (&RedScreenPicture) RedScreen();
	new (&SystemTimer) SysTimer();
}
