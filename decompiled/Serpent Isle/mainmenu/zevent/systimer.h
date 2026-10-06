#ifndef SYSTIMER_H
#define SYSTIMER_H

/* Runs the clock from construction to destruction. */
class SysTimer {
public:
	SysTimer();
	~SysTimer();
	void install();
};

extern SysTimer SystemTimer;

struct Stopwatch;
/* A countdown in timer ticks: started at start, running for length. */
struct Timer {
	unsigned long start, length;
	Timer() { start = length = 0; }
	Timer(unsigned long ticks) { start = 0; length = ticks; }
};

extern unsigned long TickCount;
inline unsigned long GetTickCount() { return TickCount; }
void SetTimerRate(unsigned hz);
void SetTimerSpeed(int rate);
unsigned long Stopwatch_getElapsed(Stopwatch *w);
void Timer_delay(Timer *t, unsigned long length);

void Stopwatch_stop(Stopwatch *w);
void Stopwatch_waitUntil(Stopwatch *w, unsigned long t);
void Timer_wait(Timer *t);
void Timer_restart(Timer *t);
void Timer_set(Timer *t, unsigned long length);
void Timer_setLength(Timer *t, unsigned long length);
unsigned long Timer_getElapsed(Timer *t);
unsigned long Timer_getRemaining(Timer *t);
unsigned long Timer_getElapsedSeconds(Timer *t);
unsigned long Timer_getRemainingSeconds(Timer *t);
unsigned char Timer_hasFinished(Timer *t);

#ifdef __cplusplus
extern "C" {
#endif
void far WaitTicks(unsigned long duration);
#ifdef __cplusplus
}
#endif

extern unsigned TimerDivisor;
extern int BiosClockCount;
extern int InterruptsPerTick;
extern int InterruptsLeft;

#endif
