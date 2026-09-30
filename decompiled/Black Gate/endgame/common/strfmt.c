/* Black Gate ENDGAME.EXE, resident segment 6 (file offsets 0x0096a9 to 0x00974e, 165 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* The shared work string and its size, read as one record. */
struct Workstring {
	char *text;
	int size;
	operator char *() { return text; }
};

extern Workstring WorkString;

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
