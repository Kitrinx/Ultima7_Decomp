/* Serpent Isle MAINMENU.EXE, resident segment 9 (file offsets 0x00dbf2 to 0x00df66, 884 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "plat.h"
#include "view.h"
#include "lowlevel.h"
#include "vooalloc.h"
#include "oops.h"
#include "u7manage.h"
#include "mouse.h"
#include "flex.h"
#include "preload.h"
#include "../zevent/u7point.h"
#include "u7point.h"

namespace MainMenu {

View *CursorTarget = 0;
int16_t ArrowCenterX = 0;
int16_t ArrowCenterY = 0;
int16_t CursorFrame = 0;
int32_t PointerShapes = 0;
int32_t CursorSaveBuffer = 0;
Rect CursorRect;
int16_t CursorDrawFlags = 0;
int16_t CursorSaveMode = 0;

/* Loads the pointer shapes from a Flex entry into Voodoo memory, with room to save what they cover. */
void LoadPointerShapes(void *self, char *flexName, int16_t entry, int16_t x, int16_t y)
{
	CursorTarget = 0;
	ArrowCenterX = x;
	ArrowCenterY = y;
	Flex flex;
	flex.open(flexName);
	FlexEntry shapes;
	flex.getEntry(entry, &shapes);
	int32_t size = shapes.size;
	PointerShapes = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (PointerShapes == 0)
		ReportOutOfVoodooMemory();
	flex.readEntryToVoodoo(&shapes, PointerShapes, 0);
	flex.close();
	CursorDrawFlags = 0x101;
	int16_t width = GetMaxFrameWidth(PointerShapes, CursorDrawFlags) + 1;
	int16_t height = GetMaxFrameHeight(PointerShapes, CursorDrawFlags) + 1;
	CursorSaveBuffer = AllocateVoodooMemory(&VoodooXmsBlock, width * height + 100);
	CursorSaveMode = 0x101;
	if (CursorSaveBuffer == 0)
		ReportOutOfVoodooMemory();
	CursorRect.set(0, 0, width, height);
	if (plat_cursor_is_hardware())
		AllocateHostCursor(width, height);
}

void MoveCursorHook(int16_t events, int16_t buttons, int16_t x, int16_t y)
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
	if (plat_cursor_is_hardware()) {
		plat_cursor_show(0);
		return;
	}
	/* No target yet: DOS used a view at address 0, whose clip box was empty. */
	if (CursorTarget != 0)
		RestoreRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
}

void DrawCursorAt(int16_t x, int16_t y)
{
	Rect r;

	if (plat_cursor_is_hardware()) {
		SetHostCursorFrame(PointerShapes, CursorFrame, CursorDrawFlags);
		plat_cursor_show(MousePresent);
	} else {
		GetFrameBounds(&r, x, y, PointerShapes, CursorFrame, CursorDrawFlags);
		CursorRect.x1 = r.x + (CursorRect.x1 - CursorRect.x);
		CursorRect.y1 = r.y + (CursorRect.y1 - CursorRect.y);
		CursorRect.x = r.x;
		CursorRect.y = r.y;
		if (CursorTarget != 0) {
			SaveRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
			if (MousePresent)
				DrawFrame((void *) CursorTarget, x, y, PointerShapes, CursorFrame, CursorDrawFlags);
		}
	}
	CursorX = x;
	CursorY = y;
}

void InstallCursorHook(MouseHandler *handler)
{
	handler->attach(MoveCursorHook, MOUSE_MOVED);
}

void ShowCursor(void)
{
	int8_t saved;

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
	int8_t saved;

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
	int8_t drawn;

	DisableCursorTracking();
	if ((drawn = IsCursorDrawn()) != 0)
		HideCursor();
	CursorTarget = buffer;
	if (drawn)
		ShowCursor();
	EnableCursorTracking();
}

void DrawCursorInto(View *buffer)
{
	View *old;

	old = CursorTarget;
	CursorTarget = buffer;
	DrawCursorAt(GetMouseX() >> 1, GetMouseY());
	CursorTarget = old;
}

void EraseCursorFrom(View *buffer)
{
	View *old;

	old = CursorTarget;
	CursorTarget = buffer;
	EraseCursor();
	CursorTarget = old;
}

}

extern "C" void ResetMainMenuU7pointGlobals(void)
{
	MainMenu::CursorTarget = 0;
	MainMenu::ArrowCenterX = 0;
	MainMenu::ArrowCenterY = 0;
	MainMenu::CursorFrame = 0;
	MainMenu::PointerShapes = 0;
	MainMenu::CursorSaveBuffer = 0;
	memset(&MainMenu::CursorRect, 0, sizeof MainMenu::CursorRect);
	MainMenu::CursorDrawFlags = 0;
	MainMenu::CursorSaveMode = 0;
}
