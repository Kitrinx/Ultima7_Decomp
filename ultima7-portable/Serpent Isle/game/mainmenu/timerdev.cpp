/* Serpent Isle MAINMENU.EXE, resident segment 14 (file offsets 0x00e750 to 0x00e785, 53 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "systimer.h"
#include "device.h"

namespace MainMenu {

uint8_t TimerDevice::start()
{
	timer->install();
	return 1;
}

/* Unhooks the clock and restores the BIOS rate. */
uint8_t TimerDevice::stop()
{
	timer->SysTimer::~SysTimer();
	return 1;
}

int16_t TimerDevice::id()
{
	return DEVICE_TIMER;
}

}
