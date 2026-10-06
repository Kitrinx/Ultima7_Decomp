/* Serpent Isle INTRO.EXE, resident segment 53 (file offsets 0x011563 to 0x011594, 49 bytes).
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
