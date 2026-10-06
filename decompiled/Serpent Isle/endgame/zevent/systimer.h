#ifndef SYSTIMER_H
#define SYSTIMER_H

/* Ticks of the system clock, 60 a second. */
extern unsigned long TickCount;

inline unsigned long GetTickCount() { return TickCount; }

/* A countdown in timer ticks: started at start, running for length. */
struct Timer {
	unsigned long start, length;
	Timer() { start = length = 0; }
	void set(unsigned long ticks) { length = ticks; start = GetTickCount(); }
	unsigned char hasFinished() { return GetTickCount() >= start + length; }
};

struct Stopwatch;

void Stopwatch_stop(Stopwatch *w);
unsigned long Stopwatch_getElapsed(Stopwatch *w);
void Stopwatch_waitUntil(Stopwatch *w, unsigned long t);
void Timer_wait(Timer *t);

/* The system clock, run by an AIL timer; it also hands out timers of other rates. */
class SysTimer {
public:
	int handle;
	SysTimer();
	~SysTimer();
	void install();
	int addTimer(void (far *callback)(), unsigned long hertz);
	void releaseTimer(int timer);
	void stopTimer(int timer);
	void startTimer(int timer);
};

extern SysTimer SystemTimer;

#endif
