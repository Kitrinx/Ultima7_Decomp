/* Pacing for the introduction: see pacing.h. */

#include "u7port.h"
#include "plat.h"
#include "pacing.h"

namespace Intro {

static uint32_t carried;            /* what fell short of a whole millisecond last time */

void SpendTime(uint32_t microseconds)
{
	uint32_t total = microseconds + carried;
	uint32_t due = plat_milliseconds() + total / 1000;

	carried = total % 1000;
	while ((int32_t) (plat_milliseconds() - due) < 0)
		plat_yield();
}

}

extern "C" void ResetIntroPacingGlobals(void)
{
	Intro::carried = 0;
}
