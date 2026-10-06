#ifndef DEVICE_H
#define DEVICE_H

struct MusicSystem;
class SysTimer;

/* The numbers DeviceList looks devices up by. */
#define DEVICE_SOUND    1
#define DEVICE_TIMER    4

/* Hardware the menu starts before use and stops on the way out. */
struct Device {
	unsigned char running;
	Device() { running = 0; }
	virtual unsigned char start() = 0;
	virtual unsigned char stop() = 0;
	virtual int id() = 0;
	unsigned char open();
	unsigned char close();
	unsigned char isOpen();
};

/* The devices, stopped in the reverse of the order they were added. */
struct DeviceList {
	Device **items;
	int count;
	int capacity;
	DeviceList();
	DeviceList(int size);
	~DeviceList();
	void init(int size);
	void add(Device *device);
	void remove(Device *device);
	Device *find(int n);
	unsigned char open(int n);
	unsigned char close(int n);
	void closeAll();
	unsigned char isOpen(int n);
};

/* The music card, with the timbre and driver files it loads. */
struct SoundDevice : Device {
	MusicSystem *music;
	char device;                /* MUSIC_DEVICE_ value, or 0 for none */
	char *timbreFile;
	char *driverFile;
	SoundDevice(MusicSystem *m, char d = 2, char *driver = 0, char *timbre = 0)
		{ device = d; driverFile = driver; timbreFile = timbre; music = m; }
	~SoundDevice();
	unsigned char start();
	unsigned char stop();
	int id();
	void setTimbreFile(char *name);
	void setDriverFile(char *name);
	void setDevice(char d) { device = d; }
};

/* The fast system timer. */
struct TimerDevice : Device {
	SysTimer *timer;
	TimerDevice(SysTimer *t) { timer = t; }
	unsigned char start();
	unsigned char stop();
	int id();
};

#endif
