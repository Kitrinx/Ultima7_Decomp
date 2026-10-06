/* Serpent Isle SI.EXE, resident segment 117 (file offsets 0x03c021 to 0x03c349, 808 bytes).
 * Borland C++ 2.0 -mm -O -G -P -1 -Y rebuilds it byte for byte as C++.
 */

#include "redscrn.h"
#include "systimer.h"
#include "init.h"
#include "ail.h"

#define TICKS_PER_SECOND 60


unsigned char TimerSystemInstalled = 0;
unsigned long TickCount = 0;
RedScreen RedScreenPicture;
SysTimer SystemTimer;

SysTimer::SysTimer()
{
	handle = -1;
}

void far AdvanceSystemClock(void)
{
	TickCount++;
}

SysTimer::~SysTimer()
{
	if (TimerSystemInstalled) {
		AIL_shutdown("bye!");
		/* Convert BIOS BCD values before updating DOS. */
		asm {
			mov ah, 2
			int 1ah
			mov al, ch
			shr al, 4
			mov dl, 10
			mul dl
			and ch, 0fh
			add ch, al
			mov al, cl
			shr al, 4
			mov dl, 10
			mul dl
			and cl, 0fh
			add cl, al
			mov al, dh
			shr al, 4
			mov dl, 10
			mul dl
			and dh, 0fh
			add dh, al
			mov ah, 2dh
			int 21h
			mov ah, 4
			int 1ah
			push dx
			mov ax, cx
			and ax, 0f0f0h
			sub cx, ax
			shr ax, 4
			mov bx, 10
			mul bx
			add cx, ax
			mov al, ch
			xor ch, ch
			mov ah, 100
			mul ah
			add cx, ax
			pop dx
			mov al, dh
			shr al, 4
			mov ah, 10
			mul ah
			and dh, 0fh
			add dh, al
			mov al, dl
			shr al, 4
			mov ah, 10
			mul ah
			and dl, 0fh
			add dl, al
			mov ah, 2bh
			int 21h
		}
	}
}

void SysTimer::install()
{
	if (!TimerSystemInstalled) {
		AIL_startup();
		handle = AIL_register_timer(AdvanceSystemClock);
		if (handle != -1) {
			AIL_set_timer_frequency(handle, 60L);
			AIL_start_timer(handle);
		} else
			FatalError("systimer.c", 208);
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

unsigned long Stopwatch_getElapsed(Stopwatch *w)
{
	if (w->running)
		return w->total + TickCount - w->startTime;
	return w->total;
}

void Stopwatch_waitUntil(Stopwatch *w, unsigned long t)
{
	while (t + w->startTime - TickCount > 0)
		;
}

void Timer_wait(Timer *t)
{
	while (t->start + t->length >= TickCount)
		;
}

void Timer_restart(Timer *t)
{
	t->start = TickCount;
}

void Timer_set(Timer *t, unsigned long length)
{
	t->length = length;
	t->start = TickCount;
}

void Timer_setLength(Timer *t, unsigned long length)
{
	t->length = length;
}

unsigned long Timer_getElapsed(Timer *t)
{
	return TickCount - t->start;
}

unsigned long Timer_getRemaining(Timer *t)
{
	return t->start + t->length - TickCount;
}

unsigned long Timer_getElapsedSeconds(Timer *t)
{
	return (TickCount - t->start) / TICKS_PER_SECOND;
}

unsigned long Timer_getRemainingSeconds(Timer *t)
{
	return (t->start + t->length - TickCount) / TICKS_PER_SECOND;
}

unsigned char Timer_hasFinished(Timer *t)
{
	return t->start + t->length <= TickCount;
}

void Timer_delay(Timer *t, unsigned long length)
{
	t->start = TickCount;
	t->length = length;
	Timer_wait(t);
}
