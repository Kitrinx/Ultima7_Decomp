/* Serpent Isle MAINMENU.EXE, resident segment 79 (file offsets 0x01a202 to 0x01a3e7, 485 bytes).
 * Borland C++ 2.0 -mm -O -G -P rebuilds it byte for byte as C++.
 * The hook and work string are the game's, so game code failing inside the menu runs the same hook.
 */

#include "u7port.h"
#include "plat.h"
#include "errors.h"

namespace Shared {

static uint8_t Reporting = 0;

void FatalError(char *fmt, ...)
{
	va_list args;
	char message[400];

	if (Reporting == 0) {
		Reporting = 1;
		if (fmt != WorkString) {
			va_start(args, fmt);
			vsnprintf(WorkString, WorkstringSize, fmt, args);
			va_end(args);
		}
		RunFatalHook();
		snprintf(message, sizeof message, "%s\nProgram terminated by code.\n", WorkString);
		plat_log(message);
		plat_exit(1);
	}
}

}

extern "C" void ResetSharedErrorsGlobals(void)
{
	Shared::Reporting = 0;
}
