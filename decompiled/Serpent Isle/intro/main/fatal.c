/* Serpent Isle INTRO.EXE, resident segment 51 (file offsets 0x011324 to 0x011398, 116 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <process.h>
#include <stdarg.h>
#include <stdio.h>
#include "biostext.h"
#include "shutdown.h"

char *HaltMessage = "Program halted by code.\n";
char *ErrorFormat = "\nError #%04x\n";
char *SubErrorFormat = "\nError #%04x, subclass %04x\n";

void FatalMessage(char *fmt, ...)
{
	va_list args;
	char message[256];

	if (fmt != message) {
		va_start(args, fmt);
		vsprintf(message, fmt, args);
	}
	RunShutdownHooks();
	PutBiosString(message);
	PutBiosString(HaltMessage);
	exit(1);
}

void FatalCode(int code)
{
	FatalMessage(ErrorFormat, code);
}

void FatalCode2(int code, int subtype)
{
	FatalMessage(SubErrorFormat, code, subtype);
}
