/* Black Gate ENDGAME.EXE, resident segment 52 (file offsets 0x010ba6 to 0x010c2e, 136 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <process.h>
#include <stdarg.h>
#include <stdio.h>
#include "biostext.h"
#include "strbuf.h"
#include "shutdown.h"

ShutdownHook far *ShutdownHooks = 0;

void ShutdownHook::shutdown()
{
}

void RunShutdownHooks()
{
	ShutdownHook far *hook;

	for (hook = ShutdownHooks; hook; hook = hook->next)
		hook->shutdown();
}

void ExitProgram(int code)
{
	RunShutdownHooks();
	exit(code);
}

/* Undoes everything, prints the message and exits. */
void ExitMessage(char *fmt, ...)
{
	va_list args;

	RunShutdownHooks();
	if (fmt != WorkBuffer) {
		va_start(args, fmt);
		vsprintf(WorkBuffer, fmt, args);
	}
	PutBiosString(WorkBuffer);
	exit(0);
}
