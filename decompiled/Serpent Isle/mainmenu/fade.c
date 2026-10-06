/* Serpent Isle MAINMENU.EXE, resident segment 27 (file offsets 0x010990 to 0x010c12, 642 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <math.h>
#include "errors.h"
#include "rgbpal.h"

#define FADE_SCALE  0x7fff

/* Sets up a fade of count components from from toward to over ticks steps. */
int Fade::prepare(unsigned char *from, unsigned char *to, int count, int ticks, int unused)
{
	int i;

	size = count;
	colors = from;
	target = to;
	if (accum == 0)
		accum = new int[size];
	if (period == 0)
		period = new int[size];
	if (step == 0)
		step = new int[size];
	if (active == 0)
		active = new unsigned char[size];
	if (accum == 0 || period == 0 || step == 0 || active == 0) {
		delete accum;
		delete period;
		delete step;
		delete active;
		FatalError("Couldn't allocate accumulator memory.");
		return 0;
	}
	rate = FADE_SCALE / ticks;
	steps = ticks;
	for (i = 0; i < size; i++) {
		active[i] = 1;
		step[i] = target[i] - colors[i];
		if (step[i] != 0) {
			period[i] = FADE_SCALE / abs(step[i]);
			accum[i] = 0;
			if (step[i] > 0)
				step[i] = 1;
			else
				step[i] = -1;
		} else
			active[i] = 0;
	}
	return 1;
}

/* Moves the fade one tick; on the last one, lands every component on its target. */
int Fade::advance()
{
	int total = size;
	unsigned char *live = active;
	int *periods = period;
	int *sums = accum;
	int delta = rate;
	unsigned char *color = colors;
	int *dir = step;
	int i;

	if (steps-- != 0) {
		asm {
			push    di
			push    si
			mov     cx, 0
		}
next:
		asm {
			mov     bx, live
			add     bx, cx
			mov     al, [bx]
			cmp     al, 0
			je      skip
			mov     bx, periods
			add     bx, cx
			add     bx, cx
			mov     dx, [bx]
			mov     bx, sums
			add     bx, cx
			add     bx, cx
			mov     ax, delta
			add     [bx], ax
		}
again:
		asm {
			mov     ax, [bx]
			sub     ax, dx
			jb      skip
			mov     [bx], ax
			mov     si, color
			add     si, cx
			mov     di, dir
			add     di, cx
			add     di, cx
			xor     ax, ax
			mov     al, [si]
			add     ax, [di]
			mov     [si], al
			jmp     again
		}
skip:
		asm {
			inc     cx
			cmp     cx, total
			jb      next
			pop     si
			pop     di
		}
		return 1;
	}
	for (i = 0; i < size; i++)
		colors[i] = target[i];
	return 0;
}

/* Frees the work arrays. */
void Fade::release()
{
	if (accum) {
		delete accum;
		accum = 0;
	}
	if (period) {
		delete period;
		period = 0;
	}
	if (step) {
		delete step;
		step = 0;
	}
	if (active) {
		delete active;
		active = 0;
	}
}
