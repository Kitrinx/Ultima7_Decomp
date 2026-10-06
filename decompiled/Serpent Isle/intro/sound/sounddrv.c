/* Serpent Isle INTRO.EXE, resident segment 7 (file offsets 0x009bc4 to 0x00a3db, 2071 bytes).
 * Borland C++ 2.0 -mm -1 -G -P -vi- rebuilds it byte for byte as C++.
 */

#include <stdio.h>
#include <string.h>
#include "sounddrv.h"
#include "iff.h"
#include "fileio.h"
#include "shutdown.h"

char *InfoId = "INFO";
char *XdirId = "XDIR";
char *XmidId = "XMID";
char *TimbId = "TIMB";

unsigned TimbreSize;

/* A timbre file entry: which timbre, and where its data starts. */
struct TimbreEntry {
	Timbre timbre;
	long offset;
};

/* Loads and starts a driver. Zero port, irq, dma or drq takes the driver's default. */
void SoundDriver::load(char *driver, char *timbres, int port, int irq, int dma, int drq, unsigned char useDefaults)
{
	int usePort, useIrq, useDma, useDrq;
	unsigned size;
	DiskFile file;

	file.open(driver, FILE_READ);
	image.allocate(file.getLength(), FAR_MEMORY, 2, 1, "No memory for sound driver.\n");
	ReadAll(&file, image.pointer());
	handle = AIL_register_driver(image.pointer());
	if (handle == -1)
		FatalMessage("Driver %s not compatible with linked API version.\n", driver);
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
	if (!AIL_detect_device(handle, usePort, useIrq, useDma, useDrq))
		FatalMessage("Sound hardware for %s not found.\n", driver);
	AIL_init_driver(handle, usePort, useIrq, useDma, useDrq);
	if (timbres) {
		size = AIL_default_timbre_cache_size(handle);
		if (size) {
			timbreCache.allocate(size, FAR_MEMORY, 0, 1, "No memory for timbre cache.\n");
			AIL_define_timbre_cache(handle, timbreCache.pointer(), size);
		}
		timbreFile.open(timbres, FILE_READ);
	}
}

/* Loads the driver <name>.INI names, with the settings and timbre file it gives. */
void SoundDriver::loadIni(char *name)
{
	int port, irq, dma, drq;
	unsigned size;
	DiskFile file;
	char fileName[13];

	strcpy(fileName, name);
	strcat(fileName, ".INI");
	IffFile ini;
	char timbres[13];
	if (ini.open(fileName, FILE_READ)) {
		if (ini.isId("INIT")) {
			ini.enterForm();
			if (ini.findChunk("SDRV"))
				ini.read(13L, fileName);
			else
				FatalMessage("No sound driver specified in %s file.\n", fileName);
			file.open(fileName, FILE_READ);
			image.allocate(file.getLength(), FAR_MEMORY, 2, 1, "No memory for sound driver.\n");
			ReadAll(&file, image.pointer());
			handle = AIL_register_driver(image.pointer());
			if (handle == -1)
				FatalMessage("Driver %s not compatible with linked API version.\n", fileName);
			descriptor = AIL_describe_driver(handle);
			port = descriptor->default_IO;
			irq = descriptor->default_IRQ;
			dma = descriptor->default_DMA;
			drq = descriptor->default_DRQ;
			printf("Port is %d\n", port);
			printf("Interrupt is %d\n", irq);
			printf("DMA is %d\n", dma);
			printf("DRQ is %d\n", drq);
			if (ini.findChunk("INFO")) {
				port = ini.readWord();
				irq = ini.readWord();
				dma = ini.readWord();
				drq = ini.readWord();
			}
			printf("Port is %d\n", port);
			printf("Interrupt is %d\n", irq);
			printf("DMA is %d\n", dma);
			printf("DRQ is %d\n", drq);
			if (!AIL_detect_device(handle, port, irq, dma, drq))
				FatalMessage("Sound hardware for %s not found.\n", fileName);
			AIL_init_driver(handle, port, irq, dma, drq);
			if (ini.findChunk("TIMB")) {
				size = AIL_default_timbre_cache_size(handle);
				if (size) {
					timbreCache.allocate(size, FAR_MEMORY, 0, 1, "No memory for timbre cache.\n");
					AIL_define_timbre_cache(handle, timbreCache.pointer(), size);
				}
				ini.read(9L, timbres);
				strcat(timbres, ".");
				_fstrcat(timbres, descriptor->data_suffix);
				timbreFile.open(timbres, FILE_READ);
			}
		}
	} else
		FatalMessage("Couldn't find sound device .ini file.\n");
}

/* Reads a timbre from the timbre file into far memory, its size in its first word; 0 if absent. */
void far *SoundDriver::loadTimbre(Timbre timbre)
{
	TimbreEntry entry;
	void far *data;

	timbreFile.rewind();
	do {
		timbreFile.read(&entry, sizeof(entry), -1L);
		if (entry.timbre.bank == -1)
			return 0;
	} while (entry.timbre.bank != timbre.bank || entry.timbre.patch != timbre.patch);
	TimbreSize = ReadWord(&timbreFile, entry.offset);
	data = Memory.allocate(TimbreSize, FAR_MEMORY, 0, 1);
	*(unsigned far *)data = TimbreSize;
	timbreFile.read((char far *)data + 2, TimbreSize - 2, -1L);
	return data;
}

/* Gives the driver a timbre it does not have yet. */
void SoundDriver::installTimbre(Timbre timbre)
{
	void far *data;

	if (!AIL_timbre_status(handle, timbre.bank, timbre.patch)) {
		data = loadTimbre(timbre);
		AIL_install_timbre(handle, timbre.bank, timbre.patch, data);
		if (data) {
			Memory.release(&data, FAR_MEMORY);
			data = 0;
		}
	}
}

/* Installs every timbre the sequences of an XMIDI file list. */
void SoundDriver::installTimbres(char *xmidi)
{
	unsigned count = 1;
	int n;
	Timbre timbre;
	IffFile file;

	file.open(xmidi, FILE_READ);
	if (file.findForm(XdirId)) {
		if (file.findChunk(InfoId))
			count = file.readWord();
		file.leaveForm();
	}
	if (file.findList(XmidId)) {
		while (count) {
			if (file.readForm() && file.isId(XmidId)) {
				file.enterForm();
				if (file.findChunk(TimbId)) {
					n = file.readWord();
					while (n--) {
						file.read(2L, &timbre);
						installTimbre(timbre);
					}
				}
				file.leaveForm();
				count--;
			}
			file.skipChunk();
			if (file.atFileEnd())
				break;
		}
		file.leaveList();
	}
}

SoundDriver::~SoundDriver()
{
	AIL_shutdown_driver(handle, "SCSCSCFY!");
	AIL_release_driver_handle(handle);
	handle = -1;
}
