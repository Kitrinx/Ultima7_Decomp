#ifndef AILXMIDI_H
#define AILXMIDI_H

#include "aildrv.h"

/* Host alignment: the game's packing would leave these pointers unaligned. */
#pragma pack(push)
#pragma pack()

/* The synthesizer half of an AIL XMIDI driver: what the shared XMIDI interpreter calls. */
struct XmidiSynth {
	int16_t kind;                   /* AIL_KIND_MT32 or AIL_KIND_ADLIB */
	uint16_t volume;                /* a sequence's starting relative volume */
	uint8_t settle;                 /* waits 10 retraces after each round of init messages */
	void (*send)(uint8_t status, uint8_t d1, uint8_t d2);
	/* type 0xf0 or 0xf7, then the message's data; 0 to skip SysEx events */
	void (*sysex)(uint8_t type, const uint8_t *data, uint16_t length);
	void (*serve)(void);            /* after each service, or 0 */
	void (*reset)(void);
	void (*init)(void);
	void (*shutdown)(const char *signOff);
	/* the first timbre of a TIMB chunk (or 0) the synth lacks, as bank << 8 | patch, or 0xffff */
	uint16_t (*timbreRequest)(const uint8_t *timb);
	uint16_t (*timbreStatus)(uint8_t bank, uint8_t patch);
	void (*installTimbre)(uint8_t bank, uint8_t patch, const uint8_t *timbre);
	void (*protectTimbre)(uint8_t bank, uint8_t patch, uint8_t on);
	uint16_t cacheSize;
	void (*defineCache)(uint8_t *base, uint16_t size);
	void (*sysexMessage)(uint8_t a, uint8_t b, uint8_t c, const uint8_t *data, uint16_t size,
		uint16_t wait);
	void (*writeDisplay)(const char *string);
};

/* Waits retraces after an MT-32 memory write, as the driver did. */
void XmidiWait(uint16_t retraces);

extern const XmidiSynth Mt32Synth;
extern const XmidiSynth OplSynth;

#pragma pack(pop)

#endif
