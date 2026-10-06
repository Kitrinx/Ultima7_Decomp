/* Serpent Isle INTRO.EXE, resident segment 6 (file offsets 0x009b1f to 0x009bc4, 165 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "errors.h"

inline char *GetWorkString(char **text)
{
	return *text;
}

/* The argument list starts at first. */
char *FormatWorkstring(char *fmt, int first, ...)
{
	va_list args;

	if (fmt != GetWorkString(&WorkString)) {
		args = (va_list) &first;
		vsprintf(GetWorkString(&WorkString), fmt, args);
	}
	return GetWorkString(&WorkString);
}

char *FormatString(char *dest, char *fmt, int first, ...)
{
	va_list args;

	if (fmt != GetWorkString(&WorkString)) {
		args = (va_list) &first;
		vsprintf(GetWorkString(&WorkString), fmt, args);
	}
	strcpy(dest, GetWorkString(&WorkString));
	return dest;
}
