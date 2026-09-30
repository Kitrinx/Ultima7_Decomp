/* Black Gate MAINMENU.EXE modules DEVICE, SOUNDDEV and TIMERDEV: the music card and timer,
 * started and stopped as a list.
 */

#include "u7port.h"
#include "systimer.h"
#include "../shared/music.h"
#include "device.h"

namespace MainMenu {

uint8_t Device::open()
{
	if (!running)
		running = start();
	return running;
}

uint8_t Device::close()
{
	if (running)
		running = !stop();
	return !running;
}

DeviceList::DeviceList()
{
	capacity = 0;
	count = 0;
	items = 0;
}

DeviceList::DeviceList(int16_t size)
{
	init(size);
}

DeviceList::~DeviceList()
{
	closeAll();
	delete[] items;
}

void DeviceList::init(int16_t size)
{
	capacity = size;
	count = 0;
	items = 0;
	if (size)
		items = new Device *[size];
}

void DeviceList::add(Device *device)
{
	if (count < capacity)
		items[count++] = device;
}

/* The original's loop copied each slot onto itself, so removal only drops the count. */
void DeviceList::remove(Device *device)
{
	if (count != 0)
		count--;
}

Device *DeviceList::find(int16_t n)
{
	int16_t i;

	for (i = 0; i < count; i++)
		if (items[i]->id() == n)
			break;
	return i < count ? items[i] : 0;
}

uint8_t DeviceList::open(int16_t n)
{
	Device *device = find(n);

	return device->open();
}

uint8_t DeviceList::close(int16_t n)
{
	Device *device = find(n);

	return device->close();
}

void DeviceList::closeAll()
{
	int16_t i;

	for (i = count; i--; )
		items[i]->close();
}

uint8_t DeviceList::isOpen(int16_t n)
{
	Device *device = find(n);

	return device->running;
}

SoundDevice::~SoundDevice()
{
	delete[] timbreFile;
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
	strcpy(timbreFile, name);
}

void SoundDevice::setDriverFile(char *name)
{
	driverFile = new char[strlen(name) + 1];
	strcpy(driverFile, name);
}

uint8_t TimerDevice::start()
{
	timer->install();
	return 1;
}

/* Stops the clock. */
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
