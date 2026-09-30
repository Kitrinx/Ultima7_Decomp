/* Black Gate MAINMENU.EXE, resident segment 14 (file offsets 0x00df1d to 0x00df52, 53 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "systimer.h"
#include "device.h"

unsigned char TimerDevice::start()
{
	timer->install();
	return 1;
}

/* Unhooks the clock and restores the BIOS rate. */
unsigned char TimerDevice::stop()
{
	timer->SysTimer::~SysTimer();
	return 1;
}

int TimerDevice::id()
{
	return DEVICE_TIMER;
}
