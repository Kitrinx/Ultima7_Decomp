/* Short waits measured in fractions of a timer tick. */

#include "u7port.h"
#include "plat.h"
#include "systimer.h"

/* Waits duration / 65536 timer ticks, rounded up to a whole millisecond. */
extern "C" void WaitTicks(uint32_t duration)
{
	const uint64_t unitsPerSecond = (uint64_t) PLAT_TICKS_PER_SECOND * 65536;

	plat_sleep((uint32_t) (((uint64_t) duration * 1000 + unitsPerSecond - 1) / unitsPerSecond));
}
