/* Black Gate MAINMENU.EXE, resident segment 9 (file offsets 0x00d42f to 0x00d76f, 832 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "u7manage.h"
#include "mouse.h"
#include "preload.h"
#include "flex.h"
#include "u7point.h"

int CursorTarget = 0;
int ArrowCenterX = 0;
int ArrowCenterY = 0;
int CursorFrame = 0;
long PointerShapes = 0;
long CursorSaveBuffer = 0;
Rect CursorRect;
int CursorDrawFlags = 0;
int CursorSaveMode = 0;

/* Loads the pointer shapes from a Flex entry into Voodoo memory, with room to save what they cover. */
void LoadPointerShapes(void *self, char *flexName, int entry, int x, int y)
{
	CursorTarget = 0;
	ArrowCenterX = x;
	ArrowCenterY = y;
	Flex flex;
	FlexEntry shapes;
	flex.open(flexName);
	flex.getEntry(entry, &shapes);
	long size = shapes.size;
	PointerShapes = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (PointerShapes == 0)
		ReportOutOfVoodooMemory();
	flex.readEntryToVoodoo(&shapes, PointerShapes, 0);
	flex.close();
	CursorDrawFlags = 0x101;
	int width = GetMaxFrameWidth(PointerShapes, CursorDrawFlags) + 1;
	int height = GetMaxFrameHeight(PointerShapes, CursorDrawFlags) + 1;
	CursorSaveBuffer = AllocateVoodooMemory(&VoodooXmsBlock, width * height + 100);
	CursorSaveMode = 0x101;
	if (CursorSaveBuffer == 0)
		ReportOutOfVoodooMemory();
	CursorRect.x0 = 0;
	CursorRect.y0 = 0;
	CursorRect.x1 = width;
	CursorRect.y1 = height;
}

void MoveCursorHook(int events, int buttons, int x, int y)
{
	if (CursorDrawn != 0) {
		if (CursorTracking != 0) {
			EraseCursor();
			DrawCursorAt(x >> 1, y);
		}
	}
}

void EraseCursor(void)
{
	RestoreRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
}

void DrawCursorAt(int x, int y)
{
	Rect r;

	GetFrameBounds(&r, x, y, PointerShapes, CursorFrame, CursorDrawFlags);
	CursorRect.x1 = r.x0 + (CursorRect.x1 - CursorRect.x0);
	CursorRect.y1 = r.y0 + (CursorRect.y1 - CursorRect.y0);
	CursorRect.x0 = r.x0;
	CursorRect.y0 = r.y0;
	SaveRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
	if (MousePresent)
		DrawFrame((void *) CursorTarget, x, y, PointerShapes, CursorFrame, CursorDrawFlags);
	CursorX = x;
	CursorY = y;
}

void InstallCursorHook(MouseHandler *handler)
{
	handler->attach(MoveCursorHook, MOUSE_MOVED);
}

void ShowCursor(void)
{
	char saved;

	saved = CursorTracking;
	CursorTracking = 0;
	if (CursorDrawn != 0)
		EraseCursor();
	DrawCursorAt(CursorX, CursorY);
	CursorDrawn = 1;
	CursorTracking = saved;
}

void HideCursor(void)
{
	char saved;

	saved = CursorTracking;
	CursorTracking = 0;
	if (CursorDrawn != 0)
		EraseCursor();
	CursorDrawn = 0;
	CursorTracking = saved;
}

/* Moves the cursor to another view, taking it off the old one first. */
void SetCursorTarget(View *buffer)
{
	char drawn;

	CursorTracking = 0;
	if ((drawn = CursorDrawn) != 0)
		HideCursor();
	CursorTarget = (int) buffer;
	if (drawn)
		ShowCursor();
	CursorTracking = 1;
}

void DrawCursorInto(View *buffer)
{
	int old;

	old = CursorTarget;
	CursorTarget = (int) buffer;
	DrawCursorAt(GetMouseX() >> 1, GetMouseY());
	CursorTarget = old;
}

void EraseCursorFrom(View *buffer)
{
	int old;

	old = CursorTarget;
	CursorTarget = (int) buffer;
	EraseCursor();
	CursorTarget = old;
}
