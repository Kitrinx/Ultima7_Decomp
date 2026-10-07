#ifndef MT32DRV_H
#define MT32DRV_H

namespace Shared {

struct SoundDriver;

/* The MT-32 music driver's entries, in the order of the loaded driver's table. */
SoundDriver *Mt32Describe(void);
int16_t Mt32Init(void *timbres);
void Mt32Shutdown(void);
void Mt32Timer(void);
void Mt32Note(int16_t channel, int16_t note, int16_t velocity);
void Mt32Control(int16_t channel, int16_t control, int16_t value);
void Mt32Pitch(int16_t channel, int16_t low, int16_t high);
void Mt32Program(int16_t channel, int16_t program);
void Mt32Port(int16_t port);
void Mt32Sysex(int32_t address, int16_t length, void *data);

}

#endif
