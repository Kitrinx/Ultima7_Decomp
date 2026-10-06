/* Serpent Isle ENDGAME.EXE, resident segment 88 (file offsets 0x015216 to 0x01526c, 86 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "errors.h"

static char Reporting = 0;

/* Formats the message, runs the hook, prints it and quits. A second error while reporting is ignored. */
extern "C" void far FatalError(char *fmt, ...)
{
	va_list args;

	if (Reporting == 0) {
		Reporting = 1;
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsprintf(WorkString, fmt, args);
		}
		RunFatalHook();
		printf("%s\nProgram terminated by code.\n", WorkString);
		exit(1);
	}
}
