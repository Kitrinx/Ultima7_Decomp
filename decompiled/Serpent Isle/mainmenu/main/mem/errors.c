/* Serpent Isle MAINMENU.EXE, resident segment 79 (file offsets 0x01a202 to 0x01a3e7, 485 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "errors.h"

char *WorkString = 0;
int WorkstringSize = 0;

/*
 * A static one of these sets up the shared work string before main and frees it at exit.
 * It also marks a fatal error being reported, so a second one is ignored.
 */
class WorkstringOwner {
public:
	char reporting;
	WorkstringOwner()
	{
		if (WorkString)
			delete WorkString;
		WorkstringSize = 256;
		WorkString = new char[WorkstringSize];
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

/* Formats the message, runs the hook, prints it and quits. */
void far FatalError(char *fmt, ...)
{
	va_list args;

	if (TheWorkstringOwner.reporting == 0) {
		TheWorkstringOwner.reporting = 1;
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		RunFatalHook();
		printf("%s\nProgram terminated by code.\n", WorkString);
		exit(1);
	}
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
