/* Black Gate U7.EXE, resident segment 57 (file offsets 0x021cf2 to 0x0225d3, 2273 bytes).
 * Borland C++ 2.0 -mm -O -G -P -d rebuilds it byte for byte as C++.
 */

#include <dos.h>
#include "lowlevel.h"
#include "dosio.h"
#include "vooalloc.h"
#include "chkfile.h"
#include "oops.h"
#include "easyfile.h"
#include "u7manage.h"
#include "mouse.h"
#include "combat.h"
#include "u7ibuf.h"
#include "crime.h"
#include "debug.h"
#include "preload.h"
#include "memapi.h"
#include "u7point.h"

struct View;

/* A rectangle, corners included. */
struct Rect {
	int x0, y0, x1, y1;
	Rect() {
		Rect *p = this;
		p->x0 = 0;
		p->y0 = 0;
		p->x1 = 0;
		p->y1 = 0;
	}
	int contains(int a, int b) {
		return (a >= x0) & (a <= x1) & (b >= y0) & (b <= y1);
	}
};

int CursorTarget = 0;
int ArrowCenterX = 0;
int ArrowCenterY = 0;
int CursorFrame = 0;
int PointerFrameCount = 0;
unsigned char CursorBase = 0;
unsigned CursorCenterZ = 0;
long PointerShapes = 0;
long CursorSaveBuffer = 0;
Rect CursorRect;
int CursorDrawFlags = 0;
int CursorSaveMode = 0;
unsigned char CardinalArrowsOnly = 0;
int ArrowLength = 0;
unsigned char CursorFrozen = 0;
int CursorSaveSize = 0;
unsigned char unused_global_3 = 0;

void LoadPointerShapes(void *self, int x, int y)
{
	CursorFrozen = 0;
	CursorTarget = 0;
	ArrowCenterX = x;
	ArrowCenterY = y;
	CursorBase = 8;
	CardinalArrowsOnly = 0;
	CursorCenterZ = 0;
	DataFile f(BuildPath(StaticPath, "pointers.shp", 0), 1);
	int size = f.getLength();
	void far *data = AllocateFarHeap(size, 0);
	if (data == 0)
		ReportOutOfFarMemory();
	if (f.read(data, size) != size)
		ReportFileNotFound(BuildPath(StaticPath, "pointers.shp", 0));
	PointerShapes = AllocateVoodooMemory(&VoodooXmsBlock, size);
	if (PointerShapes == 0)
		ReportOutOfVoodooMemory();
	CopyFarToLinear(PointerShapes, data, size);
	CursorDrawFlags = 0x101;
	int width = GetMaxFrameWidth((long)data, 0);
	int height = GetMaxFrameHeight((long)data, 0);
	PointerFrameCount = GetShapeFrameCount(PointerShapes, 0x11);
	CursorSaveSize = (width + 1) * (height + 1);
	CursorSaveBuffer = AllocateVoodooMemory(&VoodooXmsBlock, CursorSaveSize);
	if (CursorSaveBuffer == 0)
		ReportOutOfVoodooMemory();
	FillLinear(CursorSaveBuffer, 0, CursorSaveSize, 0x100);
	CursorSaveMode = 1;
	CursorRect.x0 = 0;
	CursorRect.y0 = 0;
	CursorRect.x1 = width;
	CursorRect.y1 = height;
	FreeFarHeap(data);
}

int AbsoluteInt(int value)
{
	if (value < 0)
		return -value;
	return value;
}

char CheckCursorGuard(void)
{
	long base;
	int i;

	base = base = CursorSaveBuffer - 50;   /* assigned twice, as in the original */
	for (i = 0; i < 50; i++)
		if (PeekByte(base + i))
			return 0;
	/* starts past its limit, so this never runs */
	for (i = CursorSaveSize - 50; i < 50; i++)
		if (PeekByte(base + i))
			return 0;
	return 1;
}

int GetArrowLength(int x, int y)
{
	Rect inner, outer;
	int cx, cy;

	cx = 148 - CursorCenterZ * 4;
	cy = 88 - CursorCenterZ * 4;
	inner.x0 = (cx + 10) >> 1;
	inner.y0 = (cy + 10) >> 1;
	inner.x1 = (cx + 310) >> 1;
	inner.y1 = (cy + 190) >> 1;
	outer.x0 = 10;
	outer.y0 = 10;
	outer.x1 = 310;
	outer.y1 = 190;
	if (!outer.contains(x, y))
		ArrowLength = 3;
	else if (!inner.contains(x, y))
		ArrowLength = 2;
	else
		ArrowLength = 1;
	if ((unsigned char) (CombatGroups.battleMusic | AvatarInCombat) && ArrowLength == 3)
		ArrowLength--;
	return ArrowLength;
}

int GetCursorFrame(int x, int y)
{
	int dx = 0, dy = 0;
	int slope;
	int frame;

	if (!MousePresent && (CursorBase == 8 || CursorBase == 32))
		CursorBase = 48;
	if (CursorBase < 0 || CursorBase > 49)
		frame = 0;
	else switch (CursorBase) {
	case 8:
	case 32:
		ArrowCenterX = 148 - CursorCenterZ * 4;
		ArrowCenterY = 88 - CursorCenterZ * 4;
		dx = x - ArrowCenterX;
		dy = y - ArrowCenterY;
		GetArrowLength(x, y);
		if (dx == 0)
			frame = dy < 0 ? 0 : 4;
		else {
			slope = dy * 8 / dx;
			if (!CardinalArrowsOnly) {
				if (slope < -2 && slope > -12)
					frame = dx < 0 ? 5 : 1;
				else if (slope <= -12 || slope >= 12)
					frame = dy <= 0 ? 0 : 4;
				else if (slope < 12 && slope > 2)
					frame = dy <= 0 ? 7 : 3;
				else if (slope <= 2 && slope >= -2)
					frame = dx < 0 ? 6 : 2;
				else
					frame = 0;
			} else {
				int limit = 7;

				if (slope < limit && -limit <= slope)
					frame = dx < 0 ? 6 : 2;
				else
					frame = dy < 0 ? 0 : 4;
			}
		}
		frame += (char) CursorBase;
		frame += (ArrowLength - 1) * 8;
		break;
	default:
		frame = CursorBase;
		break;
	}
	return frame < PointerFrameCount ? frame : PointerFrameCount - 1;
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
	int frame;

	frame = GetCursorFrame(x, y);
	Rect r;
	GetFrameBounds(&r, x, y, PointerShapes, frame, CursorDrawFlags);
	CursorRect.x1 = r.x0 + (CursorRect.x1 - CursorRect.x0);
	CursorRect.y1 = r.y0 + (CursorRect.y1 - CursorRect.y0);
	CursorRect.x0 = r.x0;
	CursorRect.y0 = r.y0;
	SaveRect(CursorTarget, CursorSaveBuffer, &CursorRect, CursorSaveMode);
	DrawFrame((void *)CursorTarget, x, y, PointerShapes, frame, 0x111);
	CursorX = x;
	CursorY = y;
	CursorFrame = frame;
}

void SelectMouseCursor(unsigned char cursor)
{
	if (cursor >= 0) {
		if (cursor <= 49) {
			if (AvatarDontMove != 0)
				cursor = 48;
			CursorTracking = 0;
			HideCursor();
			CursorBase = cursor;
			ShowCursor();
			CursorTracking = 1;
		}
	}
}

void AdjustCursorZ(int amount)
{
	int x, y;

	CursorTracking = 0;
	if (amount) {
		if (CursorCenterZ + amount > 15)
			CursorCenterZ = 15;
		else if (CursorCenterZ + amount < 0)
			CursorCenterZ = 0;
		else
			CursorCenterZ += amount;
		x = ClampInt(0, CursorX - amount * 4, 319);
		y = ClampInt(0, CursorY - amount * 4, 199);
		EraseCursor();
		SetMousePosition(x * 2, y);
		DrawCursorAt(x, y);
	}
	CursorTracking = 1;
}

void InstallCursorHook(MouseHandler *handler)
{
	handler->attach(MoveCursorHook, MOUSE_MOVED);
}

void ShowCursor(void)
{
	char saved;

	if (CursorFrozen == 0) {
		saved = CursorTracking;
		CursorTracking = 0;
		if (CursorDrawn != 0)
			EraseCursor();
		DrawCursorAt(GetMouseX() >> 1, GetMouseY());
		CursorDrawn = 1;
		CursorTracking = saved;
	}
}

void RedrawCursor(void)
{
	char saved;

	if (CursorFrozen == 0) {
		saved = CursorTracking;
		CursorTracking = 0;
		if (CursorDrawn != 0)
			EraseCursor();
		DrawCursorAt(CursorX, CursorY);
		CursorDrawn = 1;
		CursorTracking = saved;
	}
}

void HideCursor(void)
{
	char saved;

	if (CursorFrozen == 0) {
		saved = CursorTracking;
		CursorTracking = 0;
		if (CursorDrawn != 0)
			EraseCursor();
		CursorDrawn = 0;
		CursorTracking = saved;
	}
}

void DrawCursorInto(View *buffer)
{
	char saved;
	int old;

	if (CursorFrozen == 0) {
		saved = CursorTracking;
		CursorTracking = 0;
		old = CursorTarget;
		CursorTarget = (int) buffer;
		DrawCursorAt(GetMouseX() >> 1, GetMouseY());
		CursorTarget = old;
		CursorTracking = saved;
	}
}

void SetCursorTarget(View *buffer)
{
	CursorTarget = (int) buffer;
}

void EraseCursorFrom(View *buffer)
{
	int old;

	if (CursorFrozen == 0) {
		old = CursorTarget;
		CursorTarget = (int) buffer;
		EraseCursor();
		CursorTarget = old;
	}
}

void ShowCursorAt(int x, int y)
{
	CursorTracking = 0;
	if (CursorDrawn != 0)
		EraseCursor();
	DrawCursorAt(x, y);
	CursorDrawn = 1;
	CursorTracking = 1;
}
