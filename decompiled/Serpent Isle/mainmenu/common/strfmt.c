/* Serpent Isle MAINMENU.EXE, resident segment 29 (file offsets 0x011060 to 0x0110ea, 138 bytes).
 * Borland C++ 2.0 -mm -O -1 -P -vi- -d rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "dosio.h"
#include "fileutil.h"

extern "C" {

/* printf into WorkString; the argument list starts at first */
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

}
