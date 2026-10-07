/* Serpent Isle MAINMENU.EXE, resident segment 13 (file offsets 0x00e663 to 0x00e750, 237 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include <string.h>
#include "oops.h"
#include "../shared/music.h"
#include "device.h"

namespace MainMenu {

SoundDevice::~SoundDevice()
{
	if (timbreFile)
		delete[] timbreFile;
	if (driverFile)
		delete[] driverFile;
}

uint8_t SoundDevice::start()
{
	music->start(device, driverFile, timbreFile);
	return 1;
}

uint8_t SoundDevice::stop()
{
	music->stop();
	return 1;
}

int16_t SoundDevice::id()
{
	return DEVICE_SOUND;
}

void SoundDevice::setTimbreFile(char *name)
{
	timbreFile = new char[strlen(name) + 1];
	if (timbreFile == 0)
		ReportOutOfNearMemory();
	strcpy(timbreFile, name);
}

void SoundDevice::setDriverFile(char *name)
{
	driverFile = new char[strlen(name) + 1];
	if (driverFile == 0)
		ReportOutOfNearMemory();
	strcpy(driverFile, name);
}

}
