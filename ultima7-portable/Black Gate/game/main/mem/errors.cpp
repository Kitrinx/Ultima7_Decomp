/* Black Gate U7.EXE, resident segment 127 (file offsets 0x03dcdd to 0x03de7b, 414 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * Folder inferred from compiler flags and link order.
 */

#include "u7port.h"
#include <new>
#include "dosio.h"
#include "init.h"
#include "errors.h"

char *WorkString = 0;
int16_t WorkstringSize = 0;

/* A static one of these sets up the shared work string before main and frees it at exit. */
class WorkstringOwner {
	int8_t unused;
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

void ReportError(int16_t code)
{
	FatalError("Error 0x%04X\n", (uint16_t) code);
}

void ReportErrorSubtype(int16_t code, int16_t subtype)
{
	FatalError("Error 0x%04X (subtype 0x%04X)\n", (uint16_t) code, (uint16_t) subtype);
}

int16_t LargerOf(int16_t a, int16_t b)
{
	if (a > b)
		return a;
	return b;
}

int16_t SmallerOf(int16_t a, int16_t b)
{
	if (a < b)
		return a;
	return b;
}

void RaiseToAtLeast(int16_t *p, int16_t v)
{
	if (*p < v)
		*p = v;
}

void LowerToAtMost(int16_t *p, int16_t v)
{
	if (*p > v)
		*p = v;
}

int16_t ClampInt(int16_t lo, int16_t v, int16_t hi)
{
	if (v < lo)
		return lo;
	if (v > hi)
		return hi;
	return v;
}

extern "C" void ResetErrorsGlobals(void)
{
	WorkString = 0;
	WorkstringSize = 0;
	FatalHook = DefaultFatalHook;
}

extern "C" void ConstructErrorsGlobals(void)
{
	new (&TheWorkstringOwner) WorkstringOwner();
}
