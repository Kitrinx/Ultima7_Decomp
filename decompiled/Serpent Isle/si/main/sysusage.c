/* Serpent Isle SI.EXE, resident segment 32 (file offsets 0x019967 to 0x019a69, 258 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "debug.h"
#include "vooalloc.h"
#include "memapi.h"
#include "sysusage.h"
#include "memfree.h"

/* Free memory at the previous report; -1 until the first. */
unsigned LastNearFree = 0;
long LastFarFree = -1;
long LastVoodooFree = -1;

/* Logs how much memory went since the last report, under a printf-style label. */
void LogMemoryUsage(char *fmt, ...)
{
	long farFree, voodooFree;
	va_list args;
	char label[100];
	unsigned nearFree;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
	}
	strncpy(label, WorkString, 100);
	nearFree = NearMemory.getNearFree();
	farFree = GetFarHeapFree(0);
	voodooFree = VoodooXmsBlock.free;
	if (LastFarFree == -1) {
		DebugPrintf("Starting Memory\n%5un %7ldf %7ldv\n", nearFree, farFree, voodooFree);
		LastNearFree = nearFree;
		LastFarFree = farFree;
		LastVoodooFree = voodooFree;
	}
	DebugPrintf("%5un %7ldf %7ldv\n%s:\n", LastNearFree - nearFree, LastFarFree - farFree,
		LastVoodooFree - voodooFree, label);
	LastNearFree = nearFree;
	LastFarFree = farFree;
	LastVoodooFree = voodooFree;
}
