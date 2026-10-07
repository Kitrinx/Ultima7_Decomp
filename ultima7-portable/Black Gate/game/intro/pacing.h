#ifndef INTRO_PACING_H
#define INTRO_PACING_H

/*
 * How long the original machine spent on work the port does at once. Where a scene waited only
 * for the drawing to finish, its speed came from these; waits on the timer and on the display's
 * retrace keep their own time.
 *
 * The reference is a 386DX-33: INTRO.EXE in DOSBox-X on its normal core, cputype 386, at a fixed
 * 6075 cycles. Each figure was measured at 10000 cycles and scaled by 10000/6075, which a capture
 * at 6075 bears out. Times are in microseconds.
 */

/* One paint of the screen, by what it draws. */
#define PAINT_TIME_LIGHT            8230    /* a few small sprites: the title, the Guardian */
#define PAINT_TIME_DESK_SCALED      105350  /* the monitor enlarged, or the fist turned */
#define PAINT_TIME_DESK             80660   /* the monitor and the map, panning */
#define PAINT_TIME_DESK_DOWN        70780   /* the monitor's right half, the map and the desk */
#define PAINT_TIME_STONES           32920   /* the circle of stones */
#define PAINT_TIME_STONES_ENLARGED  41150   /* the circle of stones, closing in */

/* An enlarged curtain, added to the stones: each of its rows that reaches the screen, and each
 * of its pixels that shows (in nanoseconds). */
#define PAINT_TIME_CURTAIN_ROW      143
#define PAINT_TIME_CURTAIN_PIXEL_NS 1400

/* One frame of static, per pixel of the view it fills. */
#define STATIC_TIME_PER_KILOPIXEL   1730

namespace Intro {

/* Waits as long as the original machine would have been busy. */
void SpendTime(uint32_t microseconds);

}

#endif
