#ifndef SPEED_H
#define SPEED_H

void ResetFrameRate(void);
void ResetFrameTiming(void);
uint8_t ToggleFrameTiming(void);
void WaitForFrame(void);
void MeasureFrameRate(void);

extern uint32_t FrameTimeTotal;
extern uint32_t FrameCount;
struct Stopwatch;
extern Stopwatch FrameTimer;

#endif
