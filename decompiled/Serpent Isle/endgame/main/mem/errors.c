/* Serpent Isle ENDGAME.EXE, resident segment 87 (file offsets 0x015078 to 0x015216, 414 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include "errors.h"

char *WorkString = 0;
int WorkstringSize = 0;

/* A static one of these sets up the shared work string before main and frees it at exit. */
class WorkstringOwner {
	char unused;
public:
	WorkstringOwner()
	{
		if (WorkString)
			delete WorkString;
		WorkstringSize = 256;
		WorkString = new char[WorkstringSize + 1];
		WorkString[WorkstringSize] = 0xff;
	}
	~WorkstringOwner()
	{
		if (WorkString)
			delete WorkString;
		WorkString = 0;
		WorkstringSize = 0;
	}
};

static WorkstringOwner TheWorkstringOwner;

void DefaultFatalHook(void)
{
}

FatalHandler FatalHook = DefaultFatalHook;

void SetFatalHook(FatalHandler handler)
{
	FatalHook = handler;
}

FatalHandler SwapFatalHook(FatalHandler handler)
{
	FatalHandler old = FatalHook;

	FatalHook = handler;
	return old;
}

void RunFatalHook(void)
{
	if (FatalHook != DefaultFatalHook)
		(*FatalHook)();
}

void ReportError(int code)
{
	unsigned seg, ofs;

	/* the caller's return address */
	asm {
		push    ax
		mov     ax, [bp+4]
		mov     seg, ax
		mov     ax, [bp+2]
		mov     ofs, ax
		pop     ax
	}
	FatalError("Error 0x%04X at %04X:%04X\n", code, seg, ofs);
}

void ReportErrorSubtype(int code, int subtype)
{
	unsigned seg = code, ofs = code;   /* overwritten below; the original stores them too */

	/* the caller's return address */
	asm {
		push    ax
		mov     ax, [bp+4]
		mov     seg, ax
		mov     ax, [bp+2]
		mov     ofs, ax
		pop     ax
	}
	FatalError("Error 0x%04X (subtype 0x%04X) at %04X:%04X\n", code, subtype, seg, ofs);
}

int LargerOf(int a, int b)
{
	if (a > b)
		return a;
	return b;
}

int SmallerOf(int a, int b)
{
	if (a < b)
		return a;
	return b;
}

void RaiseToAtLeast(int *p, int v)
{
	if (*p < v)
		*p = v;
}

void LowerToAtMost(int *p, int v)
{
	if (*p > v)
		*p = v;
}

int ClampInt(int lo, int v, int hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}
