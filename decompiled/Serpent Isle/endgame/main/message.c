/* Serpent Isle ENDGAME.EXE, resident segment 52 (file offsets 0x0108c7 to 0x0108f8, 49 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include "biostext.h"
#include "strbuf.h"
#include "shutdown.h"

/* printf through the BIOS. */
void ShowMessage(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkBuffer) {
		va_start(args, fmt);
		vsprintf(WorkBuffer, fmt, args);
	}
	PutBiosString(WorkBuffer);
}
