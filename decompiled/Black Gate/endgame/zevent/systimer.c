/* Black Gate ENDGAME.EXE, resident segment 31 (file offsets 0x00d035 to 0x00d21f, 490 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "ail.h"
#include "shutdown.h"
#include "systimer.h"

#define TICKS_PER_SECOND    60

/* A stopwatch that can be stopped and restarted. */
struct Stopwatch {
	char running;
	unsigned long start;
	unsigned long total;
};

SysTimer SystemTimer;
static unsigned char TimerInstalled = 0;
unsigned long TickCount = 0;

static void TimerTick();

SysTimer::SysTimer()
{
	handle = -1;
}

/* Starts AIL and runs the clock on one of its timers. */
void SysTimer::install()
{
	if (!TimerInstalled) {
		AIL_startup();
		handle = AIL_register_timer(TimerTick);
		if (handle != -1) {
			AIL_set_timer_frequency(handle, TICKS_PER_SECOND);
			AIL_start_timer(handle);
		} else
			FatalMessage("Couldn't initialize system timer.\n");
		TimerInstalled = 1;
	}
}

/* Calls callback hertz times a second; -1 when no timer is free. */
int SysTimer::addTimer(void (far *callback)(), unsigned long hertz)
{
	int timer = -1;

	if (TimerInstalled) {
		timer = AIL_register_timer(callback);
		if (timer != -1) {
			AIL_set_timer_frequency(timer, hertz);
			AIL_start_timer(timer);
		}
	}
	return timer;
}

void SysTimer::releaseTimer(int timer)
{
	if (TimerInstalled)
		AIL_release_timer_handle(timer);
}

void SysTimer::stopTimer(int timer)
{
	if (TimerInstalled)
		AIL_stop_timer(timer);
}

void SysTimer::startTimer(int timer)
{
	if (TimerInstalled)
		AIL_start_timer(timer);
}

static void TimerTick()
{
	TickCount++;
}

SysTimer::~SysTimer()
{
	if (TimerInstalled)
		AIL_shutdown("SCSCSCFY!");
}

void Stopwatch_stop(Stopwatch *w)
{
	if (w->running) {
		w->total += TickCount - w->start;
		w->running = 0;
	}
}

unsigned long Stopwatch_getElapsed(Stopwatch *w)
{
	if (w->running)
		return w->total + TickCount - w->start;
	return w->total;
}

void Stopwatch_waitUntil(Stopwatch *w, unsigned long t)
{
	while (t + w->start - TickCount > 0)
		;
}

void Timer_wait(Timer *t)
{
	while (t->start + t->length >= TickCount)
		;
}
