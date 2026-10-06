/* Serpent Isle MAINMENU.EXE, resident segment 36 (file offsets 0x0122ca to 0x0125ab, 737 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "dosio.h"
#include "sysclk.h"
#include "systimer.h"
#include "init.h"

#define TICKS_PER_SECOND 60

/* A stopwatch that can be stopped and restarted. */
struct Stopwatch {
	char running;
	unsigned long start;
	unsigned long total;
};

unsigned long TickCount = 0;        /* ticks, 60 a second */

static HookRecord BiosClockHook = { 0, -1 };
static HookRecord TimerTickHook = { 0, -1 };
SysTimer SystemTimer;

SysTimer::SysTimer()
{
	TickCount = 0;
	SetTimerSpeed(1);
}

SysTimer::~SysTimer()
{
	SetTimerSpeed(0);
	UnhookInterrupt(8);                 /* both hooks on the timer vector */
	UnhookInterrupt(8);
}

void SysTimer::install()
{
	HookInterruptVector(8, (InterruptHandler)TimerTickHandler, &TimerTickHook, &TickChainVector);
}

/* Runs the timer chip at hz interrupts a second; 0 restores the BIOS rate. */
void SetTimerRate(unsigned hz)
{
	if (hz == 0) {
		/* back to the BIOS rate: divisor 0 counts 65536 */
		asm {
			pushf
			cli
			mov     ax, 0
			mov     TimerDivisor, ax
			mov     al, 36h
			out     43h, al
			mov     ax, 0
			out     40h, al
			out     40h, al
			popf
		}
	} else {
		/* divisor = 1193182 / hz, loaded low byte first */
		asm {
			pushf
			cli
			mov     ax, 34DEh
			mov     dx, 12h
			mov     bx, hz
			div     bx
			mov     TimerDivisor, ax
			mov     bx, ax
			mov     al, 36h
			out     43h, al
			mov     al, bl
			out     40h, al
			mov     al, bh
			out     40h, al
			popf
		}
	}
	if (BiosClockHook.vector == -1)
		HookInterruptVector(8, (InterruptHandler)BiosClockHandler, &BiosClockHook, &BiosClockVector);
}

/* Ticks the clock once every rate interrupts, the chip running at 60 * rate a second. */
void SetTimerSpeed(int rate)
{
	InterruptsPerTick = rate;
	InterruptsLeft = 1;
	SetTimerRate(rate * TICKS_PER_SECOND);
}

void Stopwatch_stop(Stopwatch *w)
{
	if (w->running) {
		w->total += GetTickCount() - w->start;
		w->running = 0;
	}
}

unsigned long Stopwatch_getElapsed(Stopwatch *w)
{
	if (w->running)
		return w->total + GetTickCount() - w->start;
	return w->total;
}

void Stopwatch_waitUntil(Stopwatch *w, unsigned long t)
{
	while (t + w->start - GetTickCount() > 0)
		;
}

void Timer_wait(Timer *t)
{
	while (t->start + t->length >= GetTickCount())
		;
}

void Timer_restart(Timer *t)
{
	t->start = GetTickCount();
}

void Timer_set(Timer *t, unsigned long length)
{
	t->length = length;
	t->start = GetTickCount();
}

void Timer_setLength(Timer *t, unsigned long length)
{
	t->length = length;
}

unsigned long Timer_getElapsed(Timer *t)
{
	return GetTickCount() - t->start;
}

unsigned long Timer_getRemaining(Timer *t)
{
	return t->start + t->length - GetTickCount();
}

unsigned long Timer_getElapsedSeconds(Timer *t)
{
	return (GetTickCount() - t->start) / TICKS_PER_SECOND;
}

unsigned long Timer_getRemainingSeconds(Timer *t)
{
	return (t->start + t->length - GetTickCount()) / TICKS_PER_SECOND;
}

unsigned char Timer_hasFinished(Timer *t)
{
	return t->start + t->length <= GetTickCount();
}

void Timer_delay(Timer *t, unsigned long length)
{
	t->start = GetTickCount();
	t->length = length;
	Timer_wait(t);
}

unsigned TimerDivisor = 0;          /* timer chip divisor */
int BiosClockCount = 0;             /* the divisor summed each interrupt; the BIOS clock runs when it wraps */
int InterruptsPerTick = 1;
int InterruptsLeft = 1;             /* interrupts left until the next tick */
