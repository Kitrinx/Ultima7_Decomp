#ifndef ADLIB_H
#define ADLIB_H

#ifdef __cplusplus
extern "C" {
#endif
void far ReclaimAdlibChannels(void);
int far GetAdlibVoiceState(int);
void far StopAdlibVoice(int);
void far TickAdlibSounds();
void far YieldAdlibChannels();
int far StartAdlibSound(int, int);
void far SetAdlibVolume(int, int);
void far ResetAdlib(void);
#ifdef __cplusplus
}
#endif

#endif
