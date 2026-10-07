/* Serpent Isle ENDGAME.EXE, resident segment 7 (file offsets 0x008f28 to 0x00973f, 2071 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include "u7port.h"
#include "sounddrv.h"

namespace Endgame {

uint16_t TimbreSize;

/* A timbre file entry: which timbre, and where its data starts. */
struct TimbreEntry {
	Timbre timbre;
	int32_t offset;
};

/* Loads and starts a driver. Zero port, irq, dma or drq takes the driver's default. Without the
 * hardware DOS stopped with a message; here the driver stays closed and plays nothing. */
void SoundDriver::load(char *driver, char *timbres, int16_t port, int16_t irq, int16_t dma, int16_t drq, uint8_t useDefaults)
{
	int16_t usePort, useIrq, useDma, useDrq;
	uint16_t size;
	int16_t file;
	int32_t length;
	char message[80];

	image = 0;
	timbreCache = 0;
	timbreFile = -1;
	file = plat_file_open(driver, PLAT_FILE_READ);
	length = file >= 0 ? plat_file_length(file) : 0;
	image = FarAllocate(length, 2, "No memory for sound driver.\n");
	if (file >= 0) {
		plat_file_read(file, image, length);
		plat_file_close(file);
	}
	handle = AIL_register_driver(image);
	if (handle == -1) {
		snprintf(message, sizeof message, "Driver %s not compatible with linked API version.\n", driver);
		plat_fatal(message);
	}
	descriptor = AIL_describe_driver(handle);
	if (useDefaults) {
		usePort = descriptor->default_IO;
		useIrq = descriptor->default_IRQ;
		useDma = descriptor->default_DMA;
		useDrq = descriptor->default_DRQ;
		if (port)
			usePort = port;
		if (irq)
			useIrq = irq;
		if (dma)
			useDma = dma;
		if (drq)
			useDrq = drq;
	} else {
		usePort = port;
		useIrq = irq;
		useDma = dma;
		useDrq = drq;
	}
	useDrq = useDma;
	if (!AIL_detect_device(handle, usePort, useIrq, useDma, useDrq)) {
		AIL_release_driver_handle(handle);
		handle = -1;
		return;
	}
	AIL_init_driver(handle, usePort, useIrq, useDma, useDrq);
	if (timbres) {
		size = AIL_default_timbre_cache_size(handle);
		if (size) {
			timbreCache = FarAllocate(size, 0, "No memory for timbre cache.\n");
			AIL_define_timbre_cache(handle, timbreCache, size);
		}
		timbreFile = plat_file_open(timbres, PLAT_FILE_READ);
	}
}

/* Reads a timbre from the timbre file into far memory, its size in its first word; 0 if absent. */
void *SoundDriver::loadTimbre(Timbre timbre)
{
	TimbreEntry entry;
	void *data;

	plat_file_seek(timbreFile, 0, PLAT_SEEK_SET);
	do {
		plat_file_read(timbreFile, &entry, sizeof(entry));
		if (entry.timbre.bank == -1)
			return 0;
	} while (entry.timbre.bank != timbre.bank || entry.timbre.patch != timbre.patch);
	plat_file_seek(timbreFile, entry.offset, PLAT_SEEK_SET);
	plat_file_read(timbreFile, &TimbreSize, 2);
	data = FarAllocate(TimbreSize, 0, "Out of memory.");
	*(uint16_t *)data = TimbreSize;
	plat_file_read(timbreFile, (char *)data + 2, TimbreSize - 2);
	return data;
}

/* Gives the driver a timbre it does not have yet. */
void SoundDriver::installTimbre(Timbre timbre)
{
	void *data;

	if (!AIL_timbre_status(handle, timbre.bank, timbre.patch)) {
		data = loadTimbre(timbre);
		AIL_install_timbre(handle, timbre.bank, timbre.patch, data);
		if (data) {
			FreeFarHeap(data);
			data = 0;
		}
	}
}

SoundDriver::~SoundDriver()
{
	AIL_shutdown_driver(handle, "SCSCSCFY!");
	AIL_release_driver_handle(handle);
	handle = -1;
	if (timbreFile >= 0)
		plat_file_close(timbreFile);
	if (timbreCache)
		FreeFarHeap(timbreCache);
	if (image)
		FreeFarHeap(image);
}

}

extern "C" void ResetEndgameSounddrvGlobals(void)
{
	Endgame::TimbreSize = 0;
}
