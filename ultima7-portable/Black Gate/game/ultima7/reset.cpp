/* Puts every file's globals back to their start-up values, so each program the launcher runs
 * starts as if the process were new. As at start-up, every plain value and zero is set before any
 * constructor runs, and the constructors run in link order: some add their object to a list or
 * set another file's variable. resets.inc is generated from the sources by the build.
 */

#include "u7port.h"
#include "plat.h"

#define RESET_PHASE1(name) extern "C" void name(void);
#define RESET_PHASE2(name) extern "C" void name(void);
#include "resets.inc"
#undef RESET_PHASE1
#undef RESET_PHASE2

extern "C" void ResetEnvironment(void)
{
#define RESET_PHASE1(name) name();
#define RESET_PHASE2(name)
#include "resets.inc"
#undef RESET_PHASE1
#undef RESET_PHASE2

#define RESET_PHASE1(name)
#define RESET_PHASE2(name) name();
#include "resets.inc"
#undef RESET_PHASE1
#undef RESET_PHASE2
}
