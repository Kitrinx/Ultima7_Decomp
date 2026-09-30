/* Black Gate INTRO.EXE, static.asm: television static.
 *
 * The original took its noise from the BIOS ROM at F000:0000, stepping through it by each
 * pixel's screen offset; here a fixed block of pseudo-random bytes stands in for the ROM, walked
 * the same way. Each frame takes the time the original machine took to draw it.
 */

#include "u7port.h"
#include "arena.h"
#include "view.h"
#include "pacing.h"
#include "intro.h"

namespace Intro {

static uint32_t Seed = 0x87654321;
static uint8_t Noise[0x10000];
static bool NoiseReady = false;

/* Fills the stand-in for the ROM from a fixed seed, so the static is the same every run. */
static void MakeNoise(void)
{
	uint32_t state = 0x2545f491;
	uint32_t i;

	for (i = 0; i < sizeof Noise; i++) {
		state = state * 1103515245 + 12345;
		Noise[i] = (uint8_t) (state >> 16);
	}
	NoiseReady = true;
}

/* Each pixel is the color or black, by the low bit of a noise byte; each frame starts at a new
 * place in the noise. */
void DrawStatic(View *view, int16_t frames, int16_t color)
{
	int16_t left = view->clip.x0;
	int16_t wide = view->clip.x1 + 1 - left;
	int16_t rows = view->clip.y1 + 1 - view->clip.y0;
	int32_t screenBase = LinearGet32(ScreenView.rowTable);
	uint16_t at;
	int16_t row, column;

	if (!NoiseReady)
		MakeNoise();
	for (; frames != 0; frames--) {
		Seed *= 0x10003;
		at = (uint16_t) (Seed >> 16);
		for (row = 0; row < rows; row++) {
			int32_t address = LinearGet32(view->rowTable + (view->clip.y0 + row) * 4) + left;
			uint8_t *out = LINEAR(address);
			/* the pixel's offset in the video segment, which the noise walk skips by */
			uint16_t offset = (uint16_t) (address - screenBase);

			for (column = 0; column < wide; column++, offset++) {
				uint8_t noise = Noise[at++];

				at += offset;
				*out++ = noise & 1 ? (uint8_t) color : 0;
			}
		}
		SpendTime((uint32_t) wide * rows * STATIC_TIME_PER_KILOPIXEL / 1000);
	}
}

}

extern "C" void ResetIntroStaticGlobals(void)
{
	Intro::Seed = 0x87654321;
	memset(Intro::Noise, 0, sizeof Intro::Noise);
	Intro::NoiseReady = false;
}
