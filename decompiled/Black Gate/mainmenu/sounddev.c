/* Black Gate MAINMENU.EXE, resident segment 13 (file offsets 0x00de30 to 0x00df1d, 237 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include <string.h>
#include "oops.h"
#include "music.h"
#include "device.h"

SoundDevice::~SoundDevice()
{
	if (timbreFile)
		delete timbreFile;
	if (driverFile)
		delete driverFile;
}

unsigned char SoundDevice::start()
{
	music->start(device, driverFile, timbreFile);
	return 1;
}

unsigned char SoundDevice::stop()
{
	music->stop();
	return 1;
}

int SoundDevice::id()
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
