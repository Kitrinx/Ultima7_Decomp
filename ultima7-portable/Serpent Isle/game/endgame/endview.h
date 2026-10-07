#ifndef ENDGAME_ENDVIEW_H
#define ENDGAME_ENDVIEW_H

struct View;

namespace Endgame {

/* The first pixel of a view whose rows follow one another. */
uint8_t *ViewPixels(struct ::View *view);
/* Draws a frame of a shape at a linear address, if the shape has that frame. */
void DrawShape(struct ::View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum);

}

#endif
