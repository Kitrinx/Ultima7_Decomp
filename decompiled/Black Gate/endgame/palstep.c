/* Black Gate ENDGAME.EXE, resident segment 25 (file offsets 0x00c6ed to 0x00c84d, 352 bytes).
 * Borland C++ 2.0 -mm -O -1 -Z -P rebuilds it byte for byte as C++.
 */

int FadeStarting = 1;
unsigned char *FadeError;
unsigned char *FadeDelta;
signed char *FadeDirection;
unsigned FadeLargest;
unsigned FadeStepsLeft;

/* Moves each of count palette components one step closer to its target, spreading the steps so that
 * every component arrives together. Returns 0 once they have all arrived. */
int pascal StepPaletteComponents(unsigned char *current, unsigned char *target, unsigned count)
{
	unsigned i;
	int diff;

	if (FadeStarting) {
		FadeError = new unsigned char[count];
		FadeDelta = new unsigned char[count];
		FadeDirection = new signed char[count];
		if (!FadeError || !FadeDelta || !FadeDirection) {
			if (FadeError)
				delete FadeError;
			if (FadeDelta)
				delete FadeDelta;
			if (FadeDirection)
				delete FadeDirection;
			return 0;
		}
		FadeLargest = 0;
		for (i = 0; i < count; i++) {
			diff = current[i] - target[i];
			if (diff < 0) {
				diff = -diff;
				FadeDirection[i] = 1;
			} else
				FadeDirection[i] = -1;
			FadeDelta[i] = diff;
			if (diff > FadeLargest)
				FadeLargest = diff;
		}
		diff = FadeLargest / 2;
		for (i = 0; i < count; i++)
			FadeError[i] = diff;
		FadeStepsLeft = FadeLargest;
		FadeStarting = 0;
	}
	if (FadeStepsLeft-- == 0) {
		delete FadeError;
		delete FadeDelta;
		delete FadeDirection;
		FadeStarting = 1;
		return 0;
	}
	for (i = 0; i < count; i++) {
		FadeError[i] += FadeDelta[i];
		if (FadeError[i] > FadeLargest) {
			FadeError[i] -= FadeLargest;
			current[i] += FadeDirection[i];
		}
	}
	return 1;
}
