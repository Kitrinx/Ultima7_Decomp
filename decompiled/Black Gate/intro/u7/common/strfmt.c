/* Black Gate U7.EXE, resident segment 95 (file offsets 0x03358a to 0x0335f5, 107 bytes).
 * Borland C++ 2.0 -mm -O -G rebuilds it byte for byte.
 * Folder inferred from Serpent Isle's link groups.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"

/* printf into WorkString; the argument list starts at first */
char *FormatWorkstring(char *fmt, int first, ...)
{
	va_list args;

	if (fmt != WorkString) {
		args = (va_list) &first;
		vsprintf(WorkString, fmt, args);
	}
	return WorkString;
}

char *FormatString(char *dest, char *fmt, int first, ...)
{
	va_list args;

	if (fmt != WorkString) {
		args = (va_list) &first;
		vsprintf(WorkString, fmt, args);
	}
	strcpy(dest, WorkString);
	return dest;
}
