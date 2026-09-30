/* Black Gate shared module ERRORS, linked into MAINMENU.EXE and INTRO.EXE: the helpers' own
 * fatal error. The hook and work string are U7's, so U7 code failing inside a helper runs the
 * same hook.
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
