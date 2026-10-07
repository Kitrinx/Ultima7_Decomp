#ifndef SOUNDDRV_H
#define SOUNDDRV_H

#include "ail.h"
#include "plat.h"
#include "memapi.h"

namespace Endgame {

/* Far memory as the memory system gave it: a failure is fatal. */
static inline void *FarAllocate(int32_t size, int16_t flags, const char *what)
{
	void *block = AllocateFarHeap(size, flags);

	if (block == 0)
		plat_fatal(what);
	return block;
}

/* A timbre as AIL numbers it: the patch in the low byte, the bank in the high one. */
struct Timbre {
	int8_t patch;
	int8_t bank;
};

/* Size of the timbre last read from the timbre file. */
extern uint16_t TimbreSize;

/* An AIL driver loaded from its .ADV file, with its timbre cache and timbre file. */
struct SoundDriver {
	int16_t handle;
	drvr_desc *descriptor;
	void *image;
	void *timbreCache;
	int16_t timbreFile;
	SoundDriver(char *driver, char *timbres, int16_t port, int16_t irq, int16_t dma, int16_t drq, uint8_t useDefaults)
		{ load(driver, timbres, port, irq, dma, drq, useDefaults); }
	void load(char *driver, char *timbres, int16_t port, int16_t irq, int16_t dma, int16_t drq, uint8_t useDefaults);
	void *loadTimbre(Timbre t);
	void installTimbre(Timbre t);
	~SoundDriver();
	int16_t getHandle() { return handle; }
	int16_t type() { return descriptor->drvr_type; }
	/* Hardware that is not there leaves the driver closed. */
	uint8_t isOpen() { return handle != -1; }
};

}

#endif
