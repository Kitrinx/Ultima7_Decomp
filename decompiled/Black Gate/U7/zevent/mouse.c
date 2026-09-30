/* Black Gate U7.EXE, resident segment 35 (file offsets 0x01a73b to 0x01aac1, 902 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

/* path: ..\zevent\mouse.c */
#include <dos.h>
#include "init.h"
#include "mouse.h"

unsigned char MousePresent = 0;
int MouseButtonCount = 0;
char MouseCallbackOff = 0;
int SavedMouseMask = 0;
void far *SavedMouseCallback = 0;
int MouseHandlerCount = 0;
int MouseX;
int MouseY;
MouseFunc MouseHandlers[MOUSE_HANDLER_SLOTS];
int MouseHandlerMasks[MOUSE_HANDLER_SLOTS];

#define MOUSE_ERROR(line)   AssertFail(__FILE__, line)

/* The driver counts two columns per pixel, so (320, 100) is the middle of the screen. */
Mouse::Mouse()
{
	InitMouse(0);
	SetMousePosition(320, 100);
	MouseX = 320;
	MouseY = 100;
}

void InitMouse(char noCallback)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 0;
	int86x(0x33, &inregs, &outregs, &sregs);
	if (outregs.x.ax == -1)
		MousePresent = 1;
	else
		MousePresent = 0;
	MouseButtonCount = outregs.x.bx;
	MouseCallbackOff = noCallback;
}

void ResetMouseDriver(void)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 0;
	int86x(0x33, &inregs, &outregs, &sregs);
}

int ReadMouseButtons(void)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 3;
	int86x(0x33, &inregs, &outregs, &sregs);
	return outregs.x.bx;
}

void SetMousePosition(int x, int y)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 4;
	inregs.x.cx = x;
	inregs.x.dx = y;
	int86x(0x33, &inregs, &outregs, &sregs);
	MouseX = x;
	MouseY = y;
}

void SetMouseXRange(int lo, int hi)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 7;
	inregs.x.cx = lo;
	inregs.x.dx = hi;
	int86x(0x33, &inregs, &outregs, &sregs);
}

void SetMouseYRange(int lo, int hi)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 8;
	inregs.x.cx = lo;
	inregs.x.dx = hi;
	int86x(0x33, &inregs, &outregs, &sregs);
}

void SetMouseMickeys(int h, int v)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	inregs.x.ax = 0x0f;
	inregs.x.cx = h;
	inregs.x.dx = v;
	int86x(0x33, &inregs, &outregs, &sregs);
}

int GetMouseX(void)
{
	return MouseX;
}

int GetMouseY(void)
{
	return MouseY;
}

void PinMouseHere(void)
{
	int x, y;

	/* where the driver has the pointer now */
	asm {
		mov     ax, 3
		int     33h
		mov     x, cx
		mov     y, dx
	}
	PinMouseAt(x >> 1, y);
}

/* Pins the pointer at (x, y); the driver counts two columns per pixel. */
void PinMouseAt(int x, int y)
{
	asm {
		shl     x, 1
		mov     ax, 4
		mov     cx, x
		mov     dx, y
		int     33h
		/* then shrink both ranges to that one point */
		mov     ax, 7
		mov     cx, x
		mov     dx, x
		int     33h
		mov     ax, 8
		mov     cx, y
		mov     dx, y
		int     33h
	}
}

/* Lets the pointer roam the whole 320x200 screen again. */
void UnpinMouse(void)
{
	asm {
		mov     ax, 7
		mov     cx, 0
		mov     dx, 639
		int     33h
		mov     ax, 8
		mov     cx, 0
		mov     dx, 199
		int     33h
	}
}

MouseHandler::MouseHandler()
{
	slot = -1;
}

void MouseHandler::attach(MouseFunc fn, int mask)
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	if (MouseHandlerCount >= MOUSE_HANDLER_SLOTS)
		MOUSE_ERROR(331);
	slot = MouseHandlerCount;
	MouseHandlers[MouseHandlerCount] = fn;
	MouseHandlerMasks[MouseHandlerCount] = mask;
	MouseHandlerCount++;
	if (MouseCallbackOff == 0) {
		inregs.x.dx = FP_OFF(DispatchMouseEvents);
		sregs.es = FP_SEG(DispatchMouseEvents);
		inregs.x.ax = 0x14;
		inregs.x.cx = 0x3f;
		int86x(0x33, &inregs, &outregs, &sregs);
		if (SavedMouseCallback == 0) {
			SavedMouseMask = outregs.x.cx;
			SavedMouseCallback = MK_FP(sregs.es, outregs.x.dx);
		}
	}
}

/* The last one out gives the driver back the handler it had. */
void MouseHandler::detach()
{
	union REGS inregs, outregs;
	struct SREGS sregs;

	if (MouseCallbackOff == 0 && installed()) {
		MouseHandlers[slot] = 0;
		MouseHandlerMasks[slot] = 0;
		if (--MouseHandlerCount == 0) {
			inregs.x.dx = FP_OFF(SavedMouseCallback);
			sregs.es = FP_SEG(SavedMouseCallback);
			inregs.x.ax = 0x14;
			inregs.x.cx = SavedMouseMask;
			int86x(0x33, &inregs, &outregs, &sregs);
			SavedMouseMask = 0;
			SavedMouseCallback = 0;
		}
	}
}
