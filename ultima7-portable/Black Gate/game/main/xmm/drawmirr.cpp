#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "shapedraw.h"

/* Draw a shape frame with x and y swapped, so each span runs down a column.
 * Shape and table addresses are always linear, so flags go unused.
 */
void DrawFrameFlipped(View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum, int16_t flags)
{
	DrawShapeFrame(view, x, y, shape, frameNum, true, SHAPE_COPY, 0);
}
