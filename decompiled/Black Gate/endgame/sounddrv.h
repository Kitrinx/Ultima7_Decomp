#ifndef SOUNDDRV_H
#define SOUNDDRV_H

#include "ail.h"
#include "memsys.h"
#include "memfile.h"

/* A timbre as AIL numbers it: the patch in the low byte, the bank in the high one. */
struct Timbre {
	char patch;
	char bank;
};

/* IFF ids of the driver's .INI file and of XMIDI files. */
extern char *InfoId;
extern char *XdirId;
extern char *XmidId;
extern char *TimbId;

/* Size of the timbre last read from the timbre file. */
extern unsigned TimbreSize;

/* An AIL driver loaded from its .ADV file, with its timbre cache and timbre file. */
struct SoundDriver {
	int handle;
	drvr_desc far *descriptor;
	MemHandle image;
	MemHandle timbreCache;
	DiskFile timbreFile;
	SoundDriver(char *driver, char *timbres, int port, int irq, int dma, int drq)
		{ load(driver, timbres, port, irq, dma, drq); }
	void load(char *driver, char *timbres, int port, int irq, int dma, int drq);
	void loadIni(char *name);
	void far *loadTimbre(Timbre t);
	void installTimbre(Timbre t);
	void installTimbres(char *xmidi);
	~SoundDriver();
	int getHandle() { return handle; }
	int type() { return descriptor->drvr_type; }
};

#endif
