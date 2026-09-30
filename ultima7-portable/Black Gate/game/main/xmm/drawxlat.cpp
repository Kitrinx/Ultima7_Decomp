#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "shapedraw.h"

/* Recolor the view under a shape frame through a 256-byte table.
 * Only the frame's outline matters, not its pixel values.
 * Shape and table addresses are always linear, so flags go unused.
 */
void DrawFrameTranslated(View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	int32_t remap, int16_t flags)
{
	DrawShapeFrame(view, x, y, shape, frameNum, false, SHAPE_TRANSLATE, remap);
}
