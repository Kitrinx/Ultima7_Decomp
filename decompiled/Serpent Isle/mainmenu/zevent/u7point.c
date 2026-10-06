/* Serpent Isle MAINMENU.EXE, resident segment 9 (file offsets 0x00dbf2 to 0x00df66, 884 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "view.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "u7manage.h"
#include "mouse.h"
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
	flex.open(flexName);
	FlexEntry shapes;
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
	CursorRect.set(0, 0, width, height);
}

void MoveCursorHook(int events, int buttons, int x, int y)
{
	if (IsCursorDrawn() != 0) {
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
	CursorRect.set(r.getX(), r.getY());
	SaveRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
	if (MouseReady())
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
	DisableCursorTracking();
	if (IsCursorDrawn() != 0)
		EraseCursor();
	DrawCursorAt(CursorX, CursorY);
	SetCursorDrawn();
	CursorTracking = saved;
}

void HideCursor(void)
{
	char saved;

	saved = CursorTracking;
	DisableCursorTracking();
	if (IsCursorDrawn() != 0)
		EraseCursor();
	ClearCursorDrawn();
	CursorTracking = saved;
}

/* Moves the cursor to another view, taking it off the old one first. */
void SetCursorTarget(View *buffer)
{
	char drawn;

	DisableCursorTracking();
	if ((drawn = IsCursorDrawn()) != 0)
		HideCursor();
	CursorTarget = (int) buffer;
	if (drawn)
		ShowCursor();
	EnableCursorTracking();
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
