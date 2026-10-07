/* The pointer drawn by the host: each frame is drawn once into a buffer of its own and handed over. */

#include "u7port.h"
#include <new>
#include "plat.h"
#include "arena.h"
#include "lowlevel.h"
#include "view.h"
#include "screen.h"
#include "oops.h"
#include "u7point.h"

static View PointerImage;
static int16_t PointerImageFrame = -1;

/* Makes a buffer big enough for every pointer frame. */
void AllocateHostCursor(int16_t width, int16_t height)
{
	PointerImage.clip.x = 0;
	PointerImage.clip.y = 0;
	PointerImage.clip.x1 = width - 1;
	PointerImage.clip.y1 = height - 1;
	if (!AllocateDrawBuffer(&PointerImage, 0xff, DRAW_IN_XMS))
		ReportOutOfVoodooMemory();
	PointerImageFrame = -1;
}

/* Sends a pointer frame to the host, with its hot spot where the game would have drawn it. */
void SetHostCursorFrame(int32_t shapes, int16_t frame, int16_t flags)
{
	Rect bounds;

	if (frame == PointerImageFrame || PointerImage.rowTable.get() == 0)
		return;
	PointerImageFrame = frame;
	GetFrameBounds(&bounds, 0, 0, shapes, frame, flags);
	FillView(&PointerImage, 0xff);
	DrawFrame(&PointerImage, -bounds.x, -bounds.y, shapes, frame, 0x111);
	plat_cursor_set_shape(LINEAR(GetRowAddress(0, PointerImage.rowTable.get())),
		PointerImage.clip.x1 + 1, PointerImage.clip.y1 + 1, -bounds.x, -bounds.y);
}

extern "C" void ResetHostcursGlobals(void)
{
	memset((void *)&PointerImage, 0, sizeof(PointerImage));
	PointerImageFrame = -1;
}

extern "C" void ConstructHostcursGlobals(void)
{
	new (&PointerImage) View();
}
