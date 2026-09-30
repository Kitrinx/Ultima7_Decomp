#include "u7port.h"
#include "view.h"
#include "saveunder.h"

/* Copy a block saved by SaveRect back onto the view, clipped to the view.
 * The buffer is always linear, so flags go unused.
 */
extern "C" void RestoreRect(View *view, int32_t buffer, void *rect, int16_t flags)
{
	const Rect *r = (const Rect *)rect;

	CopySaveBlock(view, buffer, r->x0, r->y0, r->x1, r->y1, true);
}
