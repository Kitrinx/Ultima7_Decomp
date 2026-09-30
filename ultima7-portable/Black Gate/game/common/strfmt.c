/* Black Gate U7.EXE, resident segment 95 (file offsets 0x03358a to 0x0335f5, 107 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include "u7port.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"

/* printf into WorkString */
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
