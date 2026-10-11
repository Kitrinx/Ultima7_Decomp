#ifndef ADLIBDRV_H
#define ADLIBDRV_H

namespace Shared {

struct SoundDriver;

/* The AdLib music driver's entries, in the order of the loaded driver's table. */
SoundDriver *AdlibDescribe(void);
int16_t AdlibInit(void *timbres);
void AdlibShutdown(void);
void AdlibTimer(void);
void AdlibNote(int16_t channel, int16_t note, int16_t velocity);
void AdlibControl(int16_t channel, int16_t control, int16_t value);
void AdlibPitch(int16_t channel, int16_t low, int16_t high);
void AdlibProgram(int16_t channel, int16_t program);
void AdlibPort(int16_t port);
void AdlibSysex(int32_t address, int16_t length, void *data);
void AdlibReleaseVoice(int16_t voice);
void AdlibClaimVoice(int16_t voice);

}

#endif
