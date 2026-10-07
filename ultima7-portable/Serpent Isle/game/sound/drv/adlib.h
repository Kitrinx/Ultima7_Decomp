#ifndef ADLIB_H
#define ADLIB_H

#ifdef __cplusplus
extern "C" {
#endif
void ReclaimAdlibChannels(void);
int16_t GetAdlibVoiceState(int16_t);
void StopAdlibVoice(int16_t);
void TickAdlibSounds();
void YieldAdlibChannels();
int16_t StartAdlibSound(uint8_t *, int16_t);
void SetAdlibVolume(int16_t, int16_t);
void ResetAdlib(void);
#ifdef __cplusplus
}
#endif

#endif
