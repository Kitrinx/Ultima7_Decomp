/* Serpent Isle SI.EXE, resident segment 69 (file offsets 0x02d7d0 to 0x02d83b, 107 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder chosen by subsystem.
 */

#include "u7port.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"

char *FormatWorkstring(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
		va_end(args);
	}
	return WorkString;
}

char *FormatString(char *dest, char *fmt, ...)
{
	va_list args;

	if (fmt != WorkString) {
		va_start(args, fmt);
		vsprintf(WorkString, fmt, args);
		va_end(args);
	}
	strcpy(dest, WorkString);
	return dest;
}
