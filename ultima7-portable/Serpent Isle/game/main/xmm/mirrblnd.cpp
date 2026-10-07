#include "u7port.h"
#include "lowlevel.h"
#include "view.h"
#include "shapedraw.h"

/* Draw a flipped shape frame blended with the view through a translucency table.
 * Shape and table addresses are always linear, so flags go unused.
 */
void DrawFrameFlippedTranslucent(View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum,
	int32_t blend, int16_t flags)
{
	DrawShapeFrame(view, x, y, shape, frameNum, true, SHAPE_BLEND, blend);
}
