/* Serpent Isle MAINMENU.EXE, resident segment 12 (file offsets 0x00e41d to 0x00e663, 582 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "u7port.h"
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
	if (items)
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

void DeviceList::remove(Device *device)
{
	int16_t i;

	for (i = 0; i < count; i++)
		if (items[i] == device)
			break;
	/* ++i runs before the store, so each slot is copied onto itself. */
	while (i < count)
		items[i] = items[++i];
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
	int16_t i = 0;

	for (i = count; i--; )
		items[i]->close();
}

uint8_t DeviceList::isOpen(int16_t n)
{
	Device *device = find(n);

	return device->isOpen();
}

uint8_t Device::isOpen()
{
	return running;
}

}
