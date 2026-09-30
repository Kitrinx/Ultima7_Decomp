#ifndef SYSTIMER_H
#define SYSTIMER_H

struct RedScreen;

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
	uint32_t start, length;
	Timer() { start = length = 0; }
	Timer(uint32_t ticks) { start = 0; length = ticks; }
};

extern uint32_t TickCount;
extern RedScreen RedScreenPicture;
uint32_t Stopwatch_getElapsed(Stopwatch *w);
void Timer_delay(Timer *t, uint32_t length);

void Stopwatch_stop(Stopwatch *w);
void Stopwatch_waitUntil(Stopwatch *w, uint32_t t);
void Timer_wait(Timer *t);
void Timer_restart(Timer *t);
void Timer_set(Timer *t, uint32_t length);
void Timer_setLength(Timer *t, uint32_t length);
uint32_t Timer_getElapsed(Timer *t);
uint32_t Timer_getRemaining(Timer *t);
uint32_t Timer_getElapsedSeconds(Timer *t);
uint32_t Timer_getRemainingSeconds(Timer *t);
uint8_t Timer_hasFinished(Timer *t);

#ifdef __cplusplus
extern "C" {
#endif
void WaitTicks(uint32_t duration);
#ifdef __cplusplus
}
#endif

#endif
