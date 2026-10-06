#ifndef SPEED_H
#define SPEED_H

void ResetFrameRate(void);
void ResetFrameTiming(void);
unsigned char ToggleFrameTiming(void);
void WaitForFrame(void);
void MeasureFrameRate(void);

extern unsigned long FrameTimeTotal;
extern unsigned long FrameCount;
struct Stopwatch;
extern Stopwatch FrameTimer;

#endif
