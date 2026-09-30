#ifndef MAINMENU_DEVICE_H
#define MAINMENU_DEVICE_H

class SysTimer;

namespace Shared {
struct MusicSystem;
}

namespace MainMenu {

using Shared::MusicSystem;

/* The numbers DeviceList looks devices up by. */
#define DEVICE_SOUND    1
#define DEVICE_TIMER    4

/* Hardware the menu starts before use and stops on the way out. */
struct Device {
	uint8_t running;
	Device() { running = 0; }
	virtual ~Device() {}
	virtual uint8_t start() = 0;
	virtual uint8_t stop() = 0;
	virtual int16_t id() = 0;
	uint8_t open();
	uint8_t close();
};

/* The devices, stopped in the reverse of the order they were added. */
struct DeviceList {
	Device **items;
	int16_t count;
	int16_t capacity;
	DeviceList();
	DeviceList(int16_t size);
	~DeviceList();
	void init(int16_t size);
	void add(Device *device);
	void remove(Device *device);
	Device *find(int16_t n);
	uint8_t open(int16_t n);
	uint8_t close(int16_t n);
	void closeAll();
	uint8_t isOpen(int16_t n);
};

/* The music card, with the timbre and driver files it loads. */
struct SoundDevice : Device {
	MusicSystem *music;
	char device;                /* MUSIC_DEVICE_ value, or 0 for none */
	char *timbreFile;
	char *driverFile;
	SoundDevice(MusicSystem *m) { device = 2; driverFile = 0; timbreFile = 0; music = m; }
	~SoundDevice();
	uint8_t start();
	uint8_t stop();
	int16_t id();
	void setTimbreFile(char *name);
	void setDriverFile(char *name);
};

/* The system timer. */
struct TimerDevice : Device {
	SysTimer *timer;
	TimerDevice(SysTimer *t) { timer = t; }
	uint8_t start();
	uint8_t stop();
	int16_t id();
};

}

#endif
