#ifndef PALFADE_H
#define PALFADE_H

struct Stopwatch;

void CyclePalette(void);

void FadeScreenOut(int16_t ticks, int16_t);
void FadeScreenIn(int16_t ticks, int16_t);

extern uint8_t PlayerActionSuspended;
extern Stopwatch PaletteStopwatch;

#endif
