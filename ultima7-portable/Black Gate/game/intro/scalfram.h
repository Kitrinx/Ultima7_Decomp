#ifndef INTRO_SCALFRAM_H
#define INTRO_SCALFRAM_H

struct View;

namespace Intro {

/* Draws a frame of a shape scaled (in 256ths) and turned (in degrees) about its hot spot. */
void DrawScaledFrame(View *view, int16_t x, int16_t y, int32_t shape, int16_t frame, int16_t angle,
	int16_t scale, uint8_t flip);

}

#endif
