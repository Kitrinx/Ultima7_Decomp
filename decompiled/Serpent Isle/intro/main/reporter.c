/* Serpent Isle INTRO.EXE, resident segment 47 (file offsets 0x010ddf to 0x010e5f, 128 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <stdarg.h>
#include <stdio.h>
#include "strbuf.h"
#include "shutdown.h"
#include "reporter.h"

/* Adds what the object knows about itself to the message; nothing by default. */
void ErrorReporter::describe()
{
}

void ErrorReporter::error(char *fmt, ...)
{
	va_list args;

	if (fmt != WorkBuffer) {
		va_start(args, fmt);
		vsprintf(WorkBuffer, fmt, args);
	}
	message.append(WorkBuffer);
	describe();
	handler(message);
}

void ErrorReporter::errorCode(int code)
{
	error(ErrorFormat, code);
}

void ErrorReporter::errorCode2(int code, int subtype)
{
	error(SubErrorFormat, code, subtype);
}
