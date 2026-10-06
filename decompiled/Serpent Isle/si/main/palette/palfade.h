#ifndef PALFADE_H
#define PALFADE_H

struct Stopwatch;

void far CyclePalette(void);

void far FadeScreenOut(int ticks, int);
void far FadeScreenIn(int ticks, int);

extern unsigned char PlayerActionSuspended;
extern Stopwatch PaletteStopwatch;

#endif
