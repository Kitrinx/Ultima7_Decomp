#ifndef PALFADE_H
#define PALFADE_H

/* Palette fading in 8.8 fixed point: a fraction byte per component beside its color. */

#ifdef __cplusplus
extern "C" {
#endif
extern int FadeSize;
extern int FadeStepsLeft;
int far PrepareFade(unsigned char *current, unsigned char *target, int size, int steps, int *deltas);
int far StepFade(unsigned char *colors, int *deltas, unsigned char *fractions);
#ifdef __cplusplus
}
#endif

extern int FadeIdle;
extern int *FadeDeltas;
extern unsigned char *FadeFractions;
int far FadeColors(unsigned char *colors, unsigned char *target, int size, int steps);

#endif
