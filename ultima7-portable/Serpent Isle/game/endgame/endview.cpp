/* Black Gate ENDGAME.EXE: the view helpers the ending needs, over U7's views and drawing. */

#include "u7port.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"
#include "endview.h"

namespace Endgame {

uint8_t *ViewPixels(::View *view)
{
	return LINEAR(LinearGet32(view->rowTable.get()));
}

/* The ending's drawing refused a frame number at or past the end of the frame table. */
void DrawShape(::View *view, int16_t x, int16_t y, int32_t shape, int16_t frameNum)
{
	if ((uint16_t) ((frameNum + 1) * 4) < LinearGet16(shape + 4))
		DrawFrame(view, x, y, shape, frameNum, 0);
}

}
