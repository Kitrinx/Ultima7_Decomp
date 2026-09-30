/* Black Gate INTRO.EXE, one module of resident segment 6 (file offsets 0x00a753 to 0x00aa11, 702 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <math.h>
#include <alloc.h>
#include "memapi.h"
#include "view.h"
#include "lowlevel.h"

#define FADE_SCALE      65534L

int FadeStarting = 1;
unsigned far *FadeAccum;
unsigned far *FadePeriod;
int far *FadeDir;
char *FadeActive;
int FadeTicks;
int FadeRate;

/* Moves count color components one tick of a ticks-long fade from colors toward target, setting
 * the fade up on its first call. With usedOnly, only the colors the screen shows change. Returns 1
 * while the fade runs, and 0 once every component has landed on its target. */
int StepFade(int *colors, int *target, int count, int ticks, int usedOnly)
{
	int i;

	if (FadeStarting) {
		FadeAccum = (unsigned far *) AllocateFarHeap(count * sizeof(int), 0);
		FadePeriod = (unsigned far *) AllocateFarHeap(count * sizeof(int), 0);
		FadeDir = (int far *) AllocateFarHeap(count * sizeof(int), 0);
		FadeActive = (char *) malloc(count);
		if (FadeAccum == 0 || FadePeriod == 0 || FadeDir == 0 || FadeActive == 0) {
			printf("Couldn't allocate accumulator memory.");
			FreeFarHeap(FadeAccum);
			FreeFarHeap(FadePeriod);
			FreeFarHeap(FadeDir);
			free(FadeActive);
			return 0;
		}
		FadeTicks = ticks;
		if (usedOnly) {
			for (i = 0; i < count; i++)
				FadeActive[i] = 0;
			MarkUsedColors(&ScreenView, FadeActive);
		} else {
			for (i = 0; i < count; i++)
				FadeActive[i] = 1;
		}
		for (i = 0; i < count; i++) {
			FadeDir[i] = target[i] - colors[i];
			if (FadeDir[i] != 0) {
				FadeAccum[i] = 0;
				FadePeriod[i] = abs(FADE_SCALE / FadeDir[i]);
				FadeDir[i] = FadeDir[i] / abs(FadeDir[i]);
			} else
				FadePeriod[i] = -1;
		}
		FadeRate = FADE_SCALE / ticks;
		FadeStarting = 0;
	}
	if (FadeTicks-- != 0) {
		for (i = 0; i < count; i++) {
			if (FadeActive[i]) {
				FadeAccum[i] += FadeRate;
				while (FadeAccum[i] > FadePeriod[i]) {
					FadeAccum[i] -= FadePeriod[i];
					colors[i] += FadeDir[i];
				}
			}
		}
		return 1;
	}
	for (i = 0; i < count; i++)
		colors[i] = target[i];
	FreeFarHeap(FadeAccum);
	FreeFarHeap(FadePeriod);
	FreeFarHeap(FadeDir);
	free(FadeActive);
	FadeStarting = 1;
	return 0;
}
