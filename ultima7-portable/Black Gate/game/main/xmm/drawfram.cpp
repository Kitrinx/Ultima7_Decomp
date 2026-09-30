#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "shapedraw.h"

/* Draw one frame of a run-length shape, clipped to the view.
 * Shape and table addresses are always linear, so flags go unused.
 */
void DrawFrame(void *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum, int16_t flags)
{
	DrawShapeFrame((View *)view, x, y, shape, frameNum, false, SHAPE_COPY, 0);
}
