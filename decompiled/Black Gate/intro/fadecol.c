/* Black Gate MAINMENU.EXE, one module of resident segment 19 (file offsets 0x00ed72 to 0x00ee2d, 187 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "palfade.h"

int FadeIdle = 1;
int *FadeDeltas;
unsigned char *FadeFractions;

/*
 * Moves colors one step toward target, first setting up a fade of steps steps when none is
 * running. Returns 1 while the fade runs, 0 once it is over or could not start.
 */
int far FadeColors(unsigned char *colors, unsigned char *target, int size, int steps)
{
	int i;

	if (FadeIdle) {
		FadeDeltas = new int[size];
		FadeFractions = new unsigned char[size];
		if (FadeDeltas == 0 || FadeFractions == 0) {
			if (FadeDeltas)
				delete FadeDeltas;
			if (FadeFractions)
				delete FadeFractions;
			return 0;
		}
		for (i = 0; i < size; i++)
			FadeFractions[i] = 0;
		FadeIdle = 0;
		PrepareFade(colors, target, size, steps, FadeDeltas);
	}
	if (StepFade(colors, FadeDeltas, FadeFractions)) {
		FadeIdle = 1;
		delete FadeFractions;
		delete FadeDeltas;
		return 0;
	}
	return 1;
}
