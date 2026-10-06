/* Serpent Isle MAINMENU.EXE, resident segment 12 (file offsets 0x00e41d to 0x00e663, 582 bytes).
 * Borland C++ 2.0 -mm -O -1 -P rebuilds it byte for byte as C++.
 */

#include "device.h"

unsigned char Device::open()
{
	if (!running)
		running = start();
	return running;
}

unsigned char Device::close()
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

DeviceList::DeviceList(int size)
{
	init(size);
}

DeviceList::~DeviceList()
{
	closeAll();
	if (items)
		delete items;
}

void DeviceList::init(int size)
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
	int i;

	for (i = 0; i < count; i++)
		if (items[i] == device)
			break;
	/* ++i runs before the store, so each slot is copied onto itself. */
	while (i < count)
		items[i] = items[++i];
	if (count != 0)
		count--;
}

Device *DeviceList::find(int n)
{
	int i;

	for (i = 0; i < count; i++)
		if (items[i]->id() == n)
			break;
	return i < count ? items[i] : 0;
}

unsigned char DeviceList::open(int n)
{
	Device *device = find(n);

	return device->open();
}

unsigned char DeviceList::close(int n)
{
	Device *device = find(n);

	return device->close();
}

void DeviceList::closeAll()
{
	int i = 0;

	for (i = count; i--; )
		items[i]->close();
}

unsigned char DeviceList::isOpen(int n)
{
	Device *device = find(n);

	return device->isOpen();
}

unsigned char Device::isOpen()
{
	return running;
}
